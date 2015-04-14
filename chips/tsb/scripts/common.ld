
_workram_start = ORIGIN(sram);
_workram_end = ORIGIN(sram) + LENGTH(sram);

_communication_area = ORIGIN(sram) + LENGTH(sram) - 8;
_stack_top = _communication_area;

/* The high 12kB is used for boot ROM .data, .bss, stack and communication.
   May need to revisit the number later. */
_bootrom_data_area = ORIGIN(sram) + LENGTH(sram) - 12k;

OUTPUT_ARCH(arm)
EXTERN(_vectors)
SECTIONS
{
	.text : {
		_stext = ABSOLUTE(.);
		*(.vectors)
		*(.text .text.*)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.t.*)
		*(.gnu.linkonce.r.*)
		_etext = ABSOLUTE(.);
	} > rom

	_data_in_rom = ALIGN(0x4);
	.data (_bootrom_data_area): AT (_data_in_rom) {
		_sdata = ABSOLUTE(.);
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		_edata = ALIGN(4);
	} > sram

	.bss ALIGN(4): {
		_sbss = ABSOLUTE(.);
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		_ebss = ABSOLUTE(.);
	} > sram
}
