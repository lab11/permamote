#pragma once

ret_code_t icount_init(void) ;
void icount_enable(void);
void icount_disable(void);
void icount_pause(void);
void icount_resume(void);
void icount_clear(void);
uint32_t icount_get_count(void);
