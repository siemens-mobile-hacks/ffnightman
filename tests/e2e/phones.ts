import { EmulatedPhone } from './emulator.js';

export const CX70: EmulatedPhone = {
	device: 'siemens-cx70',
	fullflash: 'CX70v56lg3.bin',
	deviceName: 'SIEMENS CX70 v56',
	platform: 'SGOLD',
	partition: 'FFS',
	chunkSize: 1024,
	unicodeNames: false,
	firmwareFile: 'Pictures/Wallpaper/Beehive.jpg',
};

export const SL65: EmulatedPhone = {
	device: 'siemens-sl65',
	fullflash: 'SL65v49lg1_TIM.bin',
	deviceName: 'SIEMENS SL65 v49',
	platform: 'SGOLD',
	partition: 'FFS',
	chunkSize: 1024,
	unicodeNames: false,
	firmwareFile: 'Pictures/schiena.jpg',
};

export const S75: EmulatedPhone = {
	device: 'siemens-s75',
	fullflash: 'S75v40lg1.bin',
	deviceName: 'SIEMENS S75 v40',
	platform: 'NewSGOLD',
	partition: 'FFS_0',
	chunkSize: 2048,
	unicodeNames: true,
	firmwareFile: 'Sounds/Caribic.mid',
};

export const EL71: EmulatedPhone = {
	device: 'siemens-el71',
	fullflash: 'EL71v41lg91.bin',
	deviceName: 'SIEMENS EL71 v41',
	platform: 'NewSGOLD',
	partition: 'FFS_0',
	chunkSize: 2048,
	unicodeNames: true,
	firmwareFile: 'Pictures/Frames/frame15.png',
};
