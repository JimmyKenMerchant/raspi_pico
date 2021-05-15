SECTIONS
{
    .PEDAL_PICO_LOOPER.FLASH : {
        . = ALIGN(4096);
        __pedal_pico_looper_flash = .;
    } > FLASH
}
