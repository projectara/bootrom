
_workram_start = ORIGIN(sram);
_workram_end = ORIGIN(sram) + LENGTH(sram);
_bufram_start = ORIGIN(bufram0);
_bufram_end = ORIGIN(bufram3) + LENGTH(bufram3);

_resume_data_size = 12;
/**
 * NOTE: _communication_area_size must match COMMUNICATION_AREA_LENGTH in
 * bootrom.h
 * bootrom.h!
 */
_communication_area_size = 1024; /* includes resume address */

_resume_data = _workram_end - _resume_data_size;
_communication_area = _workram_end - _communication_area_size;
_stack_top = DEFINED(_stack_top) ?
             _stack_top :
             ORIGIN(bufram3) + LENGTH(bufram3);

/**
 * & 0xFFFFFFE0 to make sure it is aligned with 32 bytes
 * so code in boot.S for _SIMULATION can work correctly
 */
_bootrom_data_area = DEFINED(_bootrom_data_area) ?
                     _bootrom_data_area :
                     (_communication_area - _total_data_size) & 0xFFFFFFE0;

OUTPUT_ARCH(arm)
EXTERN(_vectors)
SECTIONS
{

   /*
    * bootstrap sections are meant to contain input sections only when building
    * with SPIROM+JTAG dual-image targets such as es2tsb.  In all other
    * circumstances, such as when building for ES3's actual ROM, they must be
    * empty.
    */
	.bootstrap.loader : {
		*(.bootstrap.loader)
		_spi_stext = ABSOLUTE(.);
	} > rom

	.text (_bootrom_text_area) : {
		_stext = ABSOLUTE(.);
		*(.vectors)
		*(.text .text.*)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.t.*)
		*(.gnu.linkonce.r.*)
		_etext = ABSOLUTE(.);
	} > REGION_TEXT AT > rom

	_stext_lma = LOADADDR(.text);
	_text_size = SIZEOF(.text);

	.data (_bootrom_data_area): {
		_sdata = ALIGN(4);
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		_edata = ALIGN(4);
	} > sram AT > rom

	_data_in_rom = LOADADDR(.data);

   /*
    * bootstrap sections are meant to contain input sections only when building
    * with SPIROM+JTAG dual-image targets such as es2tsb.  In all other
    * circumstances, such as when building for ES3's actual ROM, they must be
    * empty.
    */
	.bootstrap :  {
		*(.bootstrap)
	} > REGION_TEXT AT > rom

	_bootstrap_lma = LOADADDR(.bootstrap);
	_bootstrap_vma = ADDR(.bootstrap);
	_bootstrap_size = SIZEOF(.bootstrap);

	.bss ALIGN(4): {
		_sbss = ABSOLUTE(.);
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		_ebss = ABSOLUTE(.);
	} > sram

	_total_data_size = SIZEOF(.data) + _bootstrap_size + SIZEOF(.bss);
}
