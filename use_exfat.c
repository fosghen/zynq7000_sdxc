#include "xparameters.h"	/* SDK generated parameters */
#include "xsdps.h"		/* SD device driver */
#include "xil_printf.h"
#include "ff.h"
#include "xil_cache.h"
#include "xplatform_info.h"

int FfsSdPolledExample(void);

static FIL fil;		/* File object */
static FATFS fatfs;
static char FileName[32] = "Test.bin";
static char *SD_File;

// Область памяти куда будем записывать считанные данные
u8 DestinationAddress[10*1024*1024] __attribute__ ((aligned(32)));
// Область памяти откуда берём данные для записи
u8 SourceAddress[10*1024*1024] __attribute__ ((aligned(32)));
// Структура для форматирования sd-карты
MKFS_PARM mkfs_parm;

int main(void)
{
	FRESULT Res; 					// Результат выполнения функции
	UINT NumBytesRead;				// Количество прочитанных байт
	UINT NumBytesWritten;			// Количество записанных байт
	BYTE work[FF_MAX_SS];			// Рабочий буффер, для временного хранения всякого при создании файловой системы
	u32 FileSize = (10*1024*1024);  // Размер файла, который будет записываться

	TCHAR *Path = "0:/";			// Путь к диску или разделу диска

	Res = f_mount(&fatfs, Path, 0); // Монтируем устройсво

	if (Res != FR_OK) return XST_FAILURE;

	mkfs_parm.fmt = FM_EXFAT;		// Устанавливаем файловую систему eXFAT

	//	Форматирование нашего диска
	Res = f_mkfs(Path, &mkfs_parm , work, sizeof work);
	if (Res != FR_OK) return XST_FAILURE;

	SD_File = (char *)FileName;
	// Тест записи 12000 файлов в 10 МБ
	for (int i = 0; i < 12000; i++)
	{
		sprintf(FileName, "fname%d.bin", i);
		// Создаём файл с возможностью чтения и записи
		Res = f_open(&fil, SD_File, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
		if (Res) return XST_FAILURE;

		// Получаем указатель на начало файла
		Res = f_lseek(&fil, 0);
		if (Res) return XST_FAILURE;

		// Пишем данные в файл
		Res = f_write(&fil, (const void*)SourceAddress, FileSize,
				&NumBytesWritten);
		if (Res) return XST_FAILURE;

		// Закрываем файл
		Res = f_close(&fil);
		if (Res)return XST_FAILURE;

		xil_printf("%s writed\n\r", FileName);
	}

	// Тест чтения кажого 100 файла
	for (int i = 0; i < 12000; i += 100){
		sprintf(FileName, "fname%d.bin", i);

		// Открываем файл для чтения
		Res = f_open(&fil, SD_File, FA_READ);
		if (Res) return XST_FAILURE;

		// Получаем указатель на начало файла
		Res = f_lseek(&fil, 0);
		if (Res) return XST_FAILURE;

		// Читаем в нашу память
		Res = f_read(&fil, (void*)DestinationAddress, FileSize,
				&NumBytesRead);
		if (Res)return XST_FAILURE;

		xil_printf("%s: ", FileName);

		for (int j = 0; j < 10; j ++) xil_printf("%u, ", DestinationAddress[j]);
		xil_printf("\r\n");

		// Закрываем файл
		Res = f_close(&fil);
		if (Res) return XST_FAILURE;
	}

	return XST_SUCCESS;
}

