# BTstack port for STM32 F4 Discovery Board with CC256x

This port uses the STM32 F4 Discovery Board with TI's CC256XEM ST Adapter Kit that allows to plug in a CC256xB or CC256xC Bluetooth module.
STCubeMX was used to provide the HAL and initialize the device. Finally, Eclipse CDT + GNU ARM Eclipse projects for all examples are provided.
GNU ARM Eclipse OpenOCD was used to flash and debug the examples.

## Hardware

STM32 Development kit and adapter for CC256x module:
- [STM32 F4 Discovery Board](http://www.st.com/en/evaluation-tools/stm32f4discovery.html)
- [CC256xEM Bluetooth Adatper Kit for ST](https://store.ti.com/CC256XEM-STADAPT-CC256xEM-Bluetooth-Adapter-Kit-P45158.aspx)

CC256x Bluetooth module:
- [CC2564B Dual-mode Bluetooth® Controller Evaluation Module](https://store.ti.com/cc2564modnem.aspx)
- [CC2564C Dual-mode Bluetooth® Controller Evaluation Module](https://store.ti.com/CC256XCQFN-EM-CC2564C-Dual-Mode-Bluetooth-Controller-Evaluation-Module-P51277.aspx)

The module with the older CC2564B is around USD 20, while the one with the new CC2564C costs around USD 60. The projects are configured for the CC2564B. When using the CC2564C, *bluetooth_init_cc2564C_1.0.c* should be use as cc256x_init_script, see ./create_examples.py.

## Software

To compile and flash the examples, Eclipse CDT with GNU ARM Eclipse is used. Please follow the installation instructions here: http://gnuarmeclipse.github.io
You also need to install the GNU ARM Eclipse [OpenOCD debugger](http://gnuarmeclipse.github.io/openocd/install/) plug-in.

## Create Example Projects

Eclipse requires a separate project per example. To create all example projects, navigate to this directory (btstack/port/stm32-f4discovery-cc256x) and run:

	./create_examples.py

This will create an example subfolder with one project per example. Note: it currently requires about 500 MB.

## Import Example Project
In Eclipse CDT, select "File -> Import...", then choose General->Existing Project into Workspace and click Next>. In the "Import Projects" dialog, select either the generated examples folder to import all projects or pick a single example.

## Run Example Project
Build one of the examples by clicking on the 'Hammer' icon, then start it via the provided Debug Launch configuration "example-debug". Note: It's not clear why there are two debug configurations for an example project sometimes.

## Debug output
printf is routed to USART2. To get the console output, connect PA2 (USART2 TX) of the Discovery board to an USB-2-UART adapter and open a terminal at 115200.

In src/btstack_config.h resp. in example/btstack_config.h of the generated projects, additional debug information can be enabled by uncommenting ENABLE_LOG_INFO.

Also, the full packet log can be enabled in src/port.c resp. btstack/port/stm32-f4discovery-cc256x/src/port.c by uncommenting the hci_dump_open(..) line. The console output can then be converted into .pklg files for OS X PacketLogger or WireShark by running tool/create_packet_log.py

## GATT Database
In BTstack, the GATT Database is defined via the .gatt file in the example folder. Before it can be used, run the provided update_gatt_db.sh script in each project folder.

Note: In theory, this can be integrated into the Eclipse project, in fact, it's easy to configure it as an Eclipse Builder, but it got tricky to correctly add it to the create_examples.py tool. So, whenever you're updating the .gatt file, please run ./update_gatt_db.sh manually.

## TODOs
  - move unmodified STM32 Cube files provided by ST32CubeMx into 3rd-party/STM32
  - link to sources instead of copying into every example project
  - support on-board DAC in A2DP Sink Demo

## Maintainer Notes - Updating The Port
This paragraph is mainly for the maintainers. It describes the steps to make changes to the CubeMX configuration and how to update the Eclipse template after that.

### Update cubemx-f4discovery-cc256x from STM32CubeMX project
- open ST32CubeMX
- load project file *cubemx-f4discovery-cc256x/cubemx-f4discovery-cc256x.ioc*
- generate code and documentation
- patch generated sources: ./patch-cubemx-project.sh

### Update local Eclipse project from cubemx-f4discovery-cc256x
This uses local Eclipse project generated manually from STM32 template provided by GNU ARM Eclipse using the [CubeMX Importer](https://github.com/cnoviello/CubeMXImporter)

    ./import_cubemx_into_eclipse.sh

### Update eclipse-template from local Eclipse project

	./update_from_eclipse.sh



## Adding A2DP Audio Sink
Cirrus Logic CS43L22. 
Various STM examples using older STD driver or newer HAL driver, but not easy to integrate with STM32CubeMX
Tutorial with STD Driver: http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio 
Got schematics
We need I2C and I2S port

### Information Gathering
I2S3
I2C1

#define I2S3_WS_PIN 	GPIO_Pin_4   //port A
#define I2S3_MCLK_PIN 	GPIO_Pin_7   //port C
#define I2S3_SCLK_PIN 	GPIO_Pin_10  //port C
#define I2S3_SD_PIN 	GPIO_Pin_12  //port C
#define CODEC_RESET_PIN GPIO_Pin_4  //port D
#define I2C_SCL_PIN		GPIO_Pin_6  //port B
#define I2C_SDA_PIN		GPIO_Pin_9  //port B

The provided BSP fully initialized the audio codec. Trying to use STM32CubeMX just breaks functionality. We ingore the audio codec in STM32CubeMX, resp. we keep its project file as it is.

Necessary patches:
- #define HAL_SPI_MODULE_ENABLED
- #define HAL_I2C_MODULE_ENABLED
- #define HAL_I2S_MODULE_ENABLED
- add i2c + i2s c+h files
- add DMA1_Stream7_IRQHandler to audio.c



