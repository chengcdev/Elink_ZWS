
#ifndef _RF433_H_
#define _RF433_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*rf433_logic_notify_t)(int fincId, char *data, int size);


OSStatus rf433_logic_init(rf433_logic_notify_t rf433_callback);





#ifdef __cplusplus
}
#endif

#endif /* _ML_RF433_H_ */
