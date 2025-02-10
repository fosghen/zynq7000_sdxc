#include "xparameters.h"	/* SDK generated parameters */
#include "xsdps.h"		/* SD device driver */
#include "xil_printf.h"
#include "xil_cache.h"
#include "xplatform_info.h"
#include "diskio.h"


#define POLY 0xEDB88320  // Полином для crc32
u32 crc32_table[256];    // Таблица для crc32

void crc32_init();		 // Инициализация таблицы
u32 crc32_int(int data); // Получение уникального crc32 для конкретного числа

int main(void)
{
	int res;
	BYTE write_buffer[512];   // Буфер для записи
	BYTE read_buffer[512];    // Буфер для чтения
	u64 sector_count = 0;     // Количество секторов
	u32 crc32_result;		  // Что будем класть в буффер

	// Инициализация карты
	if (disk_initialize(0) != RES_OK) {
	    xil_printf("SD not init!\n\r");
	    return 1;
	}

    // Узнаём количество секторов
	res = disk_ioctl(0, GET_SECTOR_COUNT, &sector_count);
    if (res == RES_OK) {
    	xil_printf("Sectors count: %llu\r\n", sector_count);
    } else {
    	xil_printf("Error no %d in getting sector count!\r\n", res);
    }

    // Инициализируем таблицу
    crc32_init();

    // Выполняем запись в сектора со сдвигом
	for (int sector_num = 0; sector_num <= sector_count; sector_num += 111)
	{
		crc32_result = crc32_int(sector_num);
		write_buffer[0] = (crc32_result >> 24) & 0xFF;
		write_buffer[1] = (crc32_result >> 16) & 0xFF;
		write_buffer[2] = (crc32_result >> 8 ) & 0xFF;
		write_buffer[3] = (crc32_result >> 0 ) & 0xFF;

		// Записываем наш crc32 в конкретный сектор памяти
		res = disk_write(0, write_buffer, sector_num, 1);
		if (res == RES_OK) {
			xil_printf("Success write in sector %d\r\n", sector_num);
		} else {
			xil_printf("Error write in sector %d\r\n", res);
			return 1;
		}
	}

	// Цикл чтения
	for (int sector_num = 0; sector_num < 1000; sector_num++)
		{
			// Чиатем наш crc32 из конкретного сектора памяти
			res = disk_read(0, read_buffer, sector_num, 1);
			if (res != RES_OK) {
				xil_printf("Error read in sector %d\r\n", res);
			}
			crc32_result = crc32_int(sector_num);
			if ((read_buffer[0] == ((crc32_result >> 24) & 0xFF)) &&
				(read_buffer[1] == ((crc32_result >> 16) & 0xFF)) &&
				(read_buffer[2] == ((crc32_result >> 8 ) & 0xFF)) &&
				(read_buffer[3] == ((crc32_result >> 0 ) & 0xFF))){
				xil_printf("Data verified on %d\r\n", sector_num);
			}
		}
	return XST_SUCCESS;
}


void crc32_init() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ POLY;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
}


uint32_t crc32_int(int data) {
    uint8_t *bytes = (uint8_t*)&data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < sizeof(int); i++) {
        uint8_t index = (crc ^ bytes[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }

    return crc ^ 0xFFFFFFFF;
}
