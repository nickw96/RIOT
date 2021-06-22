# Test Application for `periph_gpio` without stdio

When porting new boards to RIOT it can be useful to be able to test a new `periph_gpio` driver
while no `periph_uart` or `periph_usb` driver is in place. The existing test application relies an
a was to present the user the shell - which is convenient when possible - but no option until one
transport of stdio is implemented. This test application is aimed to fill that gap.
