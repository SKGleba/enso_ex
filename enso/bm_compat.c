// from xerpis baremetal - funcs

void gpio_set_port_mode(int bus, int port, int mode)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	gpio_regs[0] = (gpio_regs[0] & ~(1 << port)) | (mode << port);
	dmb();
}

int gpio_port_read(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	return (gpio_regs[1] >> port) & 1;
}

void gpio_port_set(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	gpio_regs[2] |= 1 << port;
	gpio_regs[0xD];
	dsb();
}

void gpio_port_clear(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	gpio_regs[3] |= 1 << port;
	gpio_regs[0xD];
	dsb();
}

int gpio_query_intr(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	return (1 << port) & ((gpio_regs[0x0E] & ~gpio_regs[0x07]) |
			      (gpio_regs[0x0F] & ~gpio_regs[0x08]) |
			      (gpio_regs[0x10] & ~gpio_regs[0x09]) |
			      (gpio_regs[0x11] & ~gpio_regs[0x0A]) |
			      (gpio_regs[0x12] & ~gpio_regs[0x0B]));
}

int gpio_acquire_intr(int bus, int port)
{
	unsigned int ret;
	unsigned int mask = 1 << port;
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);
	ret = mask & ((gpio_regs[0x0E] & ~gpio_regs[0x07]) |
		      (gpio_regs[0x0F] & ~gpio_regs[0x08]) |
		      (gpio_regs[0x10] & ~gpio_regs[0x09]) |
		      (gpio_regs[0x11] & ~gpio_regs[0x0A]) |
		      (gpio_regs[0x12] & ~gpio_regs[0x0B]));
	gpio_regs[0x0E] = mask;
	gpio_regs[0x0F] = mask;
	gpio_regs[0x10] = mask;
	gpio_regs[0x11] = mask;
	gpio_regs[0x12] = mask;
	dsb();
	return ret;
}

void spi_write_start(int bus)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	while (spi_regs[0xA])
		spi_regs[0];
	spi_regs[0xB];
	spi_regs[9] = 0x600;
}

void spi_write_end(int bus)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	spi_regs[2] = 0;
	spi_regs[4] = 1;
	spi_regs[4];
	dsb();
}

void spi_write(int bus, unsigned int data)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	spi_regs[1] = data;
}

int spi_read_available(int bus)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	return spi_regs[0xA];
}

int spi_read(int bus)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	return spi_regs[0];
}

void spi_read_end(int bus)
{
	volatile unsigned int *spi_regs = SPI_REGS(bus);
	spi_regs[4] = 0;
	spi_regs[4];
	dsb();
}

static void syscon_packet_start(struct syscon_packet *packet)
{
	int i = 0;
	unsigned char cmd_size = packet->tx[2];
	unsigned char tx_total_size = cmd_size + 3;
	unsigned int offset;
	(void)offset;
	gpio_port_clear(0, GPIO_PORT_SYSCON_OUT);
	spi_write_start(0);
	if (cmd_size <= 29) {
		offset = 2;
	}
	do {
		spi_write(0, (packet->tx[i + 1] << 8) | packet->tx[i]);
		i += 2;
	} while (i < tx_total_size);
	spi_write_end(0);
	gpio_port_set(0, GPIO_PORT_SYSCON_OUT);
}

static unsigned char syscon_cmd_sync(struct syscon_packet *packet)
{
	int i = 0;
	while (!gpio_query_intr(0, GPIO_PORT_SYSCON_IN))
		;
	gpio_acquire_intr(0, GPIO_PORT_SYSCON_IN);
	while (spi_read_available(0)) {
		unsigned int data = spi_read(0);
		packet->rx[i] = data & 0xFF;
		packet->rx[i + 1] = (data >> 8) & 0xFF;
		i += 2;
	}
	spi_read_end(0);
	gpio_port_clear(0, GPIO_PORT_SYSCON_OUT);
	return packet->rx[SYSCON_RX_RESULT];
}

static void syscon_common_read(unsigned int *buffer, unsigned short cmd)
{
	struct syscon_packet packet;
	packet.tx[SYSCON_TX_CMD_LO] = cmd & 0xFF;
	packet.tx[SYSCON_TX_CMD_HI] = (cmd >> 8) & 0xFF;
	packet.tx[SYSCON_TX_LENGTH] = 1;
	memset(packet.rx, -1, sizeof(packet.rx));
	syscon_packet_start(&packet);
	syscon_cmd_sync(&packet);
	memcpy(buffer, &packet.rx[4], packet.rx[SYSCON_RX_LENGTH] - 2);
}

static void syscon_common_write(unsigned int data, unsigned short cmd, unsigned int length)
{
	unsigned int i;
	unsigned char hash, result;
	struct syscon_packet packet;
	packet.tx[SYSCON_TX_CMD_LO] = cmd & 0xFF;
	packet.tx[SYSCON_TX_CMD_HI] = (cmd >> 8) & 0xFF;
	packet.tx[SYSCON_TX_LENGTH] = length;
	packet.tx[SYSCON_TX_DATA(0)] = data & 0xFF;
	packet.tx[SYSCON_TX_DATA(1)] = (data >> 8) & 0xFF;
	packet.tx[SYSCON_TX_DATA(2)] = (data >> 16) & 0xFF;
	packet.tx[SYSCON_TX_DATA(3)] = (data >> 24) & 0xFF;
	hash = 0;
	for (i = 0; i < length + 2; i++)
		hash += packet.tx[i];
	packet.tx[2 + length] = ~hash;
	memset(&packet.tx[3 + length], -1, sizeof(packet.rx) - (3 + length));
	do {
		memset(packet.rx, -1, sizeof(packet.rx));
		syscon_packet_start(&packet);

		result = syscon_cmd_sync(&packet);
	} while (result == 0x80 || result == 0x81);
}