/* JSON_checker.h */
#ifndef DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_CHECKER_H_
#define DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_CHECKER_H_


#ifdef __cplusplus
extern "C" {
#endif
typedef struct JSON_checker_struct {
    int valid;
    int state;
    int depth;
    int top;
    int* stack;
} * JSON_checker;


extern JSON_checker new_JSON_checker(int depth);
extern int  JSON_checker_char(JSON_checker jc, unsigned char next_char);
extern int  JSON_checker_done(JSON_checker jc);


#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_DISC_DEAL_H_ */

