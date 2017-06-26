#ifndef _MOTION_H_
#define _MOTION_H_

extern void nvp6124_hi3520_viu_init(void);
extern void hi3520_init_blank_data(unsigned int ch);
extern void nvp6124_motion_init(void);
extern nvp6124_motion_area nvp6124_get_motion_info(unsigned int ch);
extern void nvp6124_motion_display(unsigned int ch, unsigned int on);
extern void nvp6124_motion_sensitivity(unsigned int sens[16]);
#endif
