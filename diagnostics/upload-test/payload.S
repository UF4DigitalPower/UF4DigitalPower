/* Deterministic data in Flash, retained by the linker. Not copied into RAM.
 * 4 KiB prefix, then deterministic constants up to the selected image size. */
.section .upload_payload,"a",%progbits
.balign 4
.set payload_word, 0x47401303
.rept ((UPLOAD_TEST_KIB - 4) * 1024 / 4)
    .set payload_word, ((payload_word * 1664525 + 1013904223) & 0xffffffff)
    .long payload_word
.endr
