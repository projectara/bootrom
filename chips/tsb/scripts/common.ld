
_workram_start = ORIGIN(sram);
_workram_end = ORIGIN(sram) + LENGTH(sram);

_resume_address_size = 8;
_communication_area_size = 200; /* except for resume address */

_resume_address = _workram_end - _resume_address_size;
_communication_area = _resume_address - _communication_area_size;
_stack_top = _communication_area;

/* The high 12kB is used for boot ROM .data, .bss, stack and communication.
   May need to revisit the number later. */
_bootrom_data_area = DEFINED(_bootrom_data_area) ?
                     _bootrom_data_area :
                     _workram_end - 12k;

OUTPUT_ARCH(arm)
EXTERN(_vectors)
SECTIONS
{

   /*
    * bootstrap sections are meant to contain input sections only when building
    * with SPIROM+JTAG dual-image targets such as es1tsb.  In all other
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
    * with SPIROM+JTAG dual-image targets such as es1tsb.  In all other
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
}
