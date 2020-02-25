/*
 * cron.c
 *
 *  Created on: 2018年6月13日
 *      Author: Administrator
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cron.h"
#include "mico.h"

#define crondlog(M, ...) //custom_log("CROND", M, ##__VA_ARGS__)

#define isdigit(a) ((unsigned)((a) - '0') <= 9)
#define xzalloc(A)    malloc(A)

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

#define ALIGN1 //__attribute__((aligned(1)))

#define config_read_ex(parser, tokens, max, min, str, flags) \
    config_read(parser, tokens, ((flags) | (((min) & 0xFF) << 8) | ((max) & 0xFF)), str)

typedef struct CronLine {
    struct CronLine *cl_Next;
    struct CronLine *cl_Pre;
    char id[16];
    int index;
    void *func;     // execute operate
    void *func_Param;
    uint32_t func_param_len;
    int cl_Pid;
    int runErr;
//    char *cl_Shell;         /* shell command                        */
//  pid_t cl_Pid;           /* running pid, 0, or armed (-1)        */
#if ENABLE_FEATURE_CROND_CALL_SENDMAIL
    int cl_MailPos;         /* 'empty file' size                    */
    smallint cl_MailFlag;   /* running pid is for mail              */
    char *cl_MailTo;    /* whom to mail results         */
#endif
    /* ordered by size, not in natural order. makes code smaller: */
    char cl_Dow[7];         /* 0-6, beginning sunday                */
    char cl_Mons[12];       /* 0-11                                 */
    char cl_Hrs[24];        /* 0-23                                 */
    char cl_Days[32];       /* 1-31                                 */
    char cl_Mins[60];       /* 0-59                                 */
} CronLine;

typedef struct parser_t {
//    FILE *fp;
    char *line;
    char *data;
//    int lineno;
} parser_t;

/*
 * Config file parser
 */
enum {
    PARSE_COLLAPSE  = 0x00010000, // treat consecutive delimiters as one
    PARSE_TRIM      = 0x00020000, // trim leading and trailing delimiters
// TODO: COLLAPSE and TRIM seem to always go in pair
    PARSE_GREEDY    = 0x00040000, // last token takes entire remainder of the line
    PARSE_MIN_DIE   = 0x00100000, // die if < min tokens found
    // keep a copy of current line
    PARSE_KEEP_COPY = 0x00200000 * 0,//ENABLE_DEBUG_CROND_OPTION,
//  PARSE_ESCAPE    = 0x00400000, // process escape sequences in tokens
    // NORMAL is:
    // * remove leading and trailing delimiters and collapse
    //   multiple delimiters into one
    // * warn and continue if less than mintokens delimiters found
    // * grab everything into last token
    PARSE_NORMAL    = PARSE_COLLAPSE | PARSE_TRIM | PARSE_GREEDY
};

static const char DowAry[] ALIGN1 =
    "sun""mon""tue""wed""thu""fri""sat"
//{"sun","mon","tue","wed","thu","fri","sat"}
    /* "Sun""Mon""Tue""Wed""Thu""Fri""Sat" */
;

static const char MonAry[] ALIGN1 =
    "jan""feb""mar""apr""may""jun""jul""aug""sep""oct""nov""dec"
//{"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"}
    /* "Jan""Feb""Mar""Apr""May""Jun""Jul""Aug""Sep""Oct""Nov""Dec" */
;

static CronLine *g_cronHead = NULL;
static CronLine *g_cronTail = NULL;
static mico_mutex_t g_cron_mutex;
static unsigned char g_cronStart = 0;

static int cron_count = 0;


static void ParseField(char *user, char *ary, int modvalue, int off,
                const char *names, char *ptr)
/* 'names' is a pointer to a set of 3-char abbreviations */
{
    char *base = ptr;
    int n1 = -1;
    int n2 = -1;

    // this can't happen due to config_read()
    /*if (base == NULL)
        return;*/

    while (1) {
        int skip = 0;

        /* Handle numeric digit or symbol or '*' */
        if (*ptr == '*') {
            n1 = 0;     /* everything will be filled */
            n2 = modvalue - 1;
            skip = 1;
            ++ptr;
        } else if (isdigit(*ptr)) {
            if (n1 < 0) {
                n1 = strtol(ptr, &ptr, 10) + off;
            } else {
                n2 = strtol(ptr, &ptr, 10) + off;
            }
            skip = 1;
        } else if (names) {
            int i;

            for (i = 0; names[i]; i += 3) {
                /* was using strncmp before... */
                //if (strncasecmp(ptr, &names[i], 3) == 0) {
                if (strncmp(ptr, &names[i], 3) == 0) {
                    ptr += 3;
                    if (n1 < 0) {
                        n1 = i / 3;
                    } else {
                        n2 = i / 3;
                    }
                    skip = 1;
                    break;
                }
            }
        }

        /* handle optional range '-' */
        if (skip == 0) {
            goto err;
        }
        if (*ptr == '-' && n2 < 0) {
            ++ptr;
            continue;
        }

        /*
         * collapse single-value ranges, handle skipmark, and fill
         * in the character array appropriately.
         */
        if (n2 < 0) {
            n2 = n1;
        }
        if (*ptr == '/') {
            skip = strtol(ptr + 1, &ptr, 10);
        }

        /*
         * fill array, using a failsafe is the easiest way to prevent
         * an endless loop
         */
        {
            int s0 = 1;
            int failsafe = 1024;

            --n1;
            do {
                n1 = (n1 + 1) % modvalue;

                if (--s0 == 0) {
                    ary[n1 % modvalue] = 1;
                    s0 = skip;
                }
                if (--failsafe == 0) {
                    goto err;
                }
            } while (n1 != n2);

        }
        if (*ptr != ',') {
            break;
        }
        ++ptr;
        n1 = -1;
        n2 = -1;
    }

    if (*ptr) {
 err:
        return;
    }
}

static void FixDayDow(CronLine *line)
{
    unsigned i;
    int weekUsed = 0;
    int daysUsed = 0;
    int flag = 0;

    for (i = 0, flag = 0; i < ARRAY_SIZE(line->cl_Dow); ++i) {
        if (line->cl_Dow[i] == 0) {
            weekUsed = 1;
        }
        else
        {
            flag = 1;
        }
    }
    if ((weekUsed==1) && (flag == 0))
    {
        weekUsed = 0;
    }

    for (i = 0, flag=0; i < ARRAY_SIZE(line->cl_Days); ++i) {
        if (line->cl_Days[i] == 0) {
            daysUsed = 1;
        }
        else
        {
            flag = 1;
        }
    }
    if ((daysUsed==1) && (flag == 0))
    {
        daysUsed = 0;
    }
    if (weekUsed != daysUsed) {
        if (weekUsed)
            memset(line->cl_Days, 0, sizeof(line->cl_Days));
        else /* daysUsed */
            memset(line->cl_Dow, 0, sizeof(line->cl_Dow));
    }
}

static char* strchrnul(const char *s, char c)
{
    while (*s && *s != c) ++s;
    return (char*)s;
}

int config_read(parser_t *parser, char **tokens, unsigned flags, const char *delims)
{
    char *line;
    int ntokens, mintokens;
    int t, len;

    ntokens = flags & 0xFF;
    mintokens = (flags & 0xFF00) >> 8;

    if (parser == NULL)
        return 0;
    if (parser->line == NULL)
    {
        return 0;
    }

    line = parser->line;

again:
    memset(tokens, 0, sizeof(tokens[0]) * ntokens);
//  config_free_data(parser);

    /* Read one line (handling continuations with backslash) */
//  line = bb_get_chunk_with_continuation(parser->fp, &len, &parser->lineno);
//  if (line == NULL)
//      return 0;
    parser->line = line;

    /* Strip trailing line-feed if any */
//  if (len && line[len-1] == '\n')
//      line[len-1] = '\0';

    /* Skip token in the start of line? */
    if (flags & PARSE_TRIM)
    {
        int pos = strspn(line, delims + 1);
        line = line + pos;
    }


    if (line[0] == '\0' || line[0] == delims[0])
        goto again;

    if (flags & PARSE_KEEP_COPY)
        parser->data = strdup(line);

    /* Tokenize the line */
    for (t = 0; *line && *line != delims[0] && t < ntokens; t++) {
        /* Pin token */
        tokens[t] = line;

        /* Combine remaining arguments? */
        if ((t != (ntokens-1)) || !(flags & PARSE_GREEDY)) {
            /* Vanilla token, find next delimiter */
            line += strcspn(line, delims[0] ? delims : delims + 1);
        } else {
            /* Combining, find comment char if any */
            line = strchrnul(line, delims[0]);

            /* Trim any extra delimiters from the end */
            if (flags & PARSE_TRIM) {
                while (strchr(delims + 1, line[-1]) != NULL)
                    line--;
            }
        }

        /* Token not terminated? */
        if (line[0] == delims[0])
            *line = '\0';
        else if (line[0] != '\0')
            *(line++) = '\0';

        /* Skip possible delimiters */
        if (flags & PARSE_COLLAPSE)
            line += strspn(line, delims + 1);
    }

    if (t < mintokens) {
//        printf("bad line %u: %d tokens found, %d needed\r\n",
//                parser->lineno, t, mintokens);

        goto again;
    }

    return t;
}

#if 0
int FAST_FUNC config_read(parser_t *parser, char **tokens, unsigned flags, const char *delims)
{
    char *line;
    int ntokens, mintokens;
    int t, len;

    ntokens = flags & 0xFF;
    mintokens = (flags & 0xFF00) >> 8;

    if (parser == NULL)
        return 0;

again:
    memset(tokens, 0, sizeof(tokens[0]) * ntokens);
    config_free_data(parser);

    /* Read one line (handling continuations with backslash) */
    line = bb_get_chunk_with_continuation(parser->fp, &len, &parser->lineno);
    if (line == NULL)
        return 0;
    parser->line = line;

    /* Strip trailing line-feed if any */
    if (len && line[len-1] == '\n')
        line[len-1] = '\0';

    /* Skip token in the start of line? */
    if (flags & PARSE_TRIM)
        line += strspn(line, delims + 1);

    if (line[0] == '\0' || line[0] == delims[0])
        goto again;

    if (flags & PARSE_KEEP_COPY)
        parser->data = xstrdup(line);

    /* Tokenize the line */
    for (t = 0; *line && *line != delims[0] && t < ntokens; t++) {
        /* Pin token */
        tokens[t] = line;

        /* Combine remaining arguments? */
        if ((t != (ntokens-1)) || !(flags & PARSE_GREEDY)) {
            /* Vanilla token, find next delimiter */
            line += strcspn(line, delims[0] ? delims : delims + 1);
        } else {
            /* Combining, find comment char if any */
            line = strchrnul(line, delims[0]);

            /* Trim any extra delimiters from the end */
            if (flags & PARSE_TRIM) {
                while (strchr(delims + 1, line[-1]) != NULL)
                    line--;
            }
        }

        /* Token not terminated? */
        if (line[0] == delims[0])
            *line = '\0';
        else if (line[0] != '\0')
            *(line++) = '\0';

#if 0 /* unused so far */
        if (flags & PARSE_ESCAPE) {
            const char *from;
            char *to;

            from = to = tokens[t];
            while (*from) {
                if (*from == '\\') {
                    from++;
                    *to++ = bb_process_escape_sequence(&from);
                } else {
                    *to++ = *from++;
                }
            }
            *to = '\0';
        }
#endif

        /* Skip possible delimiters */
        if (flags & PARSE_COLLAPSE)
            line += strspn(line, delims + 1);
    }

    if (t < mintokens) {
        bb_error_msg("bad line %u: %d tokens found, %d needed",
                parser->lineno, t, mintokens);
        if (flags & PARSE_MIN_DIE)
            xfunc_die();
        goto again;
    }

    return t;
}



void SynchronizeFile(const char *fileName)
{
    struct parser_t *parser;
    struct stat sbuf;
    int maxLines;
    char *tokens[6];



    if (1) {


        CronLine **pline;
        int n;

        file->cf_User = xstrdup(fileName);
        pline = &file->cf_LineBase;

        while (1) {
            CronLine *line;

            if (!--maxLines)
                break;
            n = config_read(parser, tokens, 6, 1, "# \t", PARSE_NORMAL | PARSE_KEEP_COPY);



            if (!n)
                break;

            /* check if line is setting MAILTO= */
            if (0 == strncmp(tokens[0], "MAILTO=", 7)) {
#if ENABLE_FEATURE_CROND_CALL_SENDMAIL
                free(mailTo);
                mailTo = (tokens[0][7]) ? xstrdup(&tokens[0][7]) : NULL;
#endif /* otherwise just ignore such lines */
                continue;
            }
            /* check if a minimum of tokens is specified */
            if (n < 6)
                continue;
            *pline = line = xzalloc(sizeof(*line));
            /* parse date ranges */
            ParseField(file->cf_User, line->cl_Mins, 60, 0, NULL, tokens[0]);
            ParseField(file->cf_User, line->cl_Hrs, 24, 0, NULL, tokens[1]);
            ParseField(file->cf_User, line->cl_Days, 32, 0, NULL, tokens[2]);
            ParseField(file->cf_User, line->cl_Mons, 12, -1, MonAry, tokens[3]);
            ParseField(file->cf_User, line->cl_Dow, 7, 0, DowAry, tokens[4]);
            /*
             * fix days and dow - if one is not "*" and the other
             * is "*", the other is set to 0, and vise-versa
             */
            FixDayDow(line);
#if ENABLE_FEATURE_CROND_CALL_SENDMAIL
            /* copy mailto (can be NULL) */
            line->cl_MailTo = xstrdup(mailTo);
#endif
            /* copy command */
            line->cl_Shell = xstrdup(tokens[5]);
            if (DebugOpt) {
                crondlog(LVL5 " command:%s", tokens[5]);
            }
            pline = &line->cl_Next;
//bb_error_msg("M[%s]F[%s][%s][%s][%s][%s][%s]", mailTo, tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5]);
        }
        *pline = NULL;

        file->cf_Next = FileBase;
        FileBase = file;

        if (maxLines == 0) {
            crondlog(WARN9 "user %s: too many lines", fileName);
        }
    }
    config_close(parser);
}

#endif

static void config_free_data(parser_t *const parser)
{
    if (PARSE_KEEP_COPY) { /* compile-time constant */
        free(parser->data);
        parser->data = NULL;
    }
}

void cron_add_task(const char *cron_id, const char *cron_expression, func_schedule_task task_deal, uint8_t *param, uint32_t param_len)
{
    struct parser_t parser = {0};
//    struct stat sbuf;
//    int maxLines;
    char *tokens[6];
    uint8_t *funParam = NULL;
    parser.line = cron_expression;
    if (param_len != 0)
    {
        funParam = (uint8_t *)malloc(param_len);
        memcpy(funParam, param, param_len);
    }

    if (1) {

        CronLine *line = NULL;
        CronLine *lineTemp = NULL;
        int n;

        line = g_cronHead;
        while(line)     // check if task is exist
        {
            if (strcmp(line->id, cron_id) == 0)
            {
                mico_rtos_lock_mutex(&g_cron_mutex);
                if (lineTemp == NULL)
                {
                    g_cronHead = line->cl_Next;
                    if (g_cronHead == NULL)
                    {
                        g_cronTail = NULL;
                    }
                }
                else
                {
                    lineTemp->cl_Next = line->cl_Next;
                }
                if (line->func_Param != NULL)
                {
                    free(line->func_Param);
                }
                free(line);
                cron_count--;
                mico_rtos_unlock_mutex(&g_cron_mutex);
                break;
            }
            lineTemp = line;
            line = line->cl_Next;
        }

//        while (1) {
            n = config_read_ex(&parser, tokens, 6, 1, "# \t", PARSE_NORMAL | PARSE_KEEP_COPY);

            if (!n)
                return;

            /* check if line is setting MAILTO= */
//            if (0 == strncmp(tokens[0], "MAILTO=", 7)) {
//#if ENABLE_FEATURE_CROND_CALL_SENDMAIL
//                free(mailTo);
//                mailTo = (tokens[0][7]) ? xstrdup(&tokens[0][7]) : NULL;
//#endif /* otherwise just ignore such lines */
//                continue;
//            }
            /* check if a minimum of tokens is specified */
            if (n < 6)
                return;

            lineTemp = xzalloc(sizeof(CronLine));
            memset(lineTemp, 0, sizeof(CronLine));

            /* parse date ranges */
            ParseField(NULL, lineTemp->cl_Mins, 60, 0, NULL, tokens[1]);
            ParseField(NULL, lineTemp->cl_Hrs, 24, 0, NULL, tokens[2]);
            ParseField(NULL, lineTemp->cl_Days, 32, 0, NULL, tokens[3]);
            ParseField(NULL, lineTemp->cl_Mons, 12, -1, MonAry, tokens[4]);
            ParseField(NULL, lineTemp->cl_Dow, 7, 0, DowAry, tokens[5]);
            /*
             * fix days and dow - if one is not "*" and the other
             * is "*", the other is set to 0, and vise-versa
             */

            FixDayDow(lineTemp);

//#if ENABLE_FEATURE_CROND_CALL_SENDMAIL
//            /* copy mailto (can be NULL) */
//            line->cl_MailTo = xstrdup(mailTo);
//#endif
            /* copy command */
            lineTemp->func = task_deal;
            lineTemp->func_Param = funParam;
            lineTemp->func_param_len = param_len;
            lineTemp->cl_Next = NULL;
            strcpy(lineTemp->id, cron_id);
            // push cron to link table
            if (g_cronHead)
            {
                g_cronTail->cl_Next = lineTemp;
                g_cronTail = lineTemp;
            }
            else
            {
                g_cronHead = lineTemp;
                g_cronTail = g_cronHead;
            }
            cron_count++;
//            pline = &line->cl_Next;
//bb_error_msg("M[%s]F[%s][%s][%s][%s][%s][%s]", mailTo, tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5]);
//        }

//        if (maxLines == 0) {
//            crondlog(WARN9 "user %s: too many lines", fileName);
//        }
            crondlog("cron_count: %d, malloc : %x, current id: %s", cron_count, lineTemp, lineTemp->id);
    }
    config_free_data(&parser);
}

void cron_del_task(const char *cron_id)
{
    CronLine *line = NULL;
    CronLine *lineTemp = NULL;

    line = g_cronHead;
    while(line)     // check if task is exist
    {
        crondlog("cron id is %s", line->id);
        // check if id is match
        if (strcmp(line->id, cron_id) == 0)
        {
            mico_rtos_lock_mutex(&g_cron_mutex);
            if (lineTemp == NULL)       // it's first time in the "while"
            {
                g_cronHead = line->cl_Next;
                if (g_cronHead == NULL)     // there is nothing in the link table
                {
                    g_cronTail = NULL;
                }
            }
            else
            {
                if (g_cronTail == line)
                {
                    g_cronTail = lineTemp;
                }
                lineTemp->cl_Next = line->cl_Next;
            }
            // free memory
            if (line->func_Param != NULL)
            {
                free(line->func_Param);
            }
            free(line);
            line = NULL;
            mico_rtos_unlock_mutex(&g_cron_mutex);
            cron_count--;
            break;
        }
        lineTemp = line;
        line = line->cl_Next;
    }
//    crondlog("g_cronHead: %x, g_cronTail: %x, cron_count: %d, del: %s", g_cronHead, g_cronTail, cron_count, cron_id);
}

/*
 * TestJobs()
 *
 * determine which jobs need to be run.  Under normal conditions, the
 * period is about a minute (one scan).  Worst case it will be one
 * hour (60 scans).
 */
static int TestJobs(time_t t1, time_t t2)
{
    int nJobs = 0;
    time_t t;

    /* Find jobs > t1 and <= t2 */

    for (t = t1 - t1 % 60; t <= t2; t += 60) {
        struct tm *tp;
        CronLine *line;
        time_t timeTemp = 0;
        if (t <= t1)
            continue;
        if (g_cronHead == NULL)
        {
            crondlog("there is no schedule");
        }
        timeTemp = t+3600*8;
        tp = localtime(&timeTemp);
        crondlog("hour: %d, minitue: %d, mtu: %d", tp->tm_hour, tp->tm_min, t2);
//        for (file = FileBase; file; file = file->cf_Next) {
//            if (DebugOpt)
//                crondlog(LVL5 "file %s:", file->cf_User);
//            if (file->cf_Deleted)
//                continue;
//            for (line = file->cf_LineBase; line; line = line->cl_Next) {
            for (line = g_cronHead; line; line = line->cl_Next) {
//                if (DebugOpt)
//                    crondlog(LVL5 " line %s", line->cl_Shell);
//                crondlog("min: %d, hour: %d, day: %d, wday: %d", line->cl_Mins[tp->tm_min], line->cl_Hrs[tp->tm_hour], line->cl_Days[tp->tm_mday], line->cl_Dow[tp->tm_wday]);
                if (line->cl_Mins[tp->tm_min] && line->cl_Hrs[tp->tm_hour]
                 && (line->cl_Days[tp->tm_mday] || line->cl_Dow[tp->tm_wday])
                 && line->cl_Mons[tp->tm_mon]
                ) {
//                    if (line->cl_Pid > 0) {
//                        crondlog(LVL8 "user %s: process already running: %s",
//                            file->cf_User, line->cl_Shell);
//                    } else if (line->cl_Pid == 0) {
                        line->cl_Pid = -1;
//                        file->cf_Ready = 1;
                        ++nJobs;
//                        crondlog("time out");
//                    }
                }
            }
//        }
    }
    return nJobs;
}

void RunJob(const char *user, CronLine *line)
{
//    struct passwd *pas;
//    pid_t pid;
//
//    /* prepare things before vfork */
//    pas = getpwnam(user);
//    if (!pas) {
//        crondlog(LVL9 "can't get uid for %s", user);
//        goto err;
//    }
//    SetEnv(pas);
//
//    /* fork as the user in question and run program */
//    pid = vfork();
//    if (pid == 0) {
//        /* CHILD */
//        /* change running state to the user in question */
//        ChangeUser(pas);
//        if (DebugOpt) {
//            crondlog(LVL5 "child running %s", DEFAULT_SHELL);
//        }
//        execl(DEFAULT_SHELL, DEFAULT_SHELL, "-c", line->cl_Shell, NULL);
//        crondlog(ERR20 "can't exec, user %s cmd %s %s %s", user,
//                 DEFAULT_SHELL, "-c", line->cl_Shell);
//        _exit(EXIT_SUCCESS);
//    }
//    if (pid < 0) {
//        /* FORK FAILED */
//        crondlog(ERR20 "can't vfork");
// err:
//        pid = 0;
//    }
    func_schedule_task task_deal = NULL;
    task_deal = line->func;
    crondlog("schedule takes effect");
    line->runErr = task_deal(line->id, line->func_Param, line->func_param_len);
    line->cl_Pid = 0;

}

static void RunJobs(void)
{
//    CronFile *file;
    CronLine *line;

//    for (file = FileBase; file; file = file->cf_Next) {
//        if (!file->cf_Ready)
//            continue;

//        file->cf_Ready = 0;
//        for (line = file->cf_LineBase; line; line = line->cl_Next) {
        for (line = g_cronHead; line; line = line->cl_Next) {
            if (line->cl_Pid >= 0)
                continue;
            RunJob(NULL, line);
//            RunJob(file->cf_User, line);
//            crondlog(LVL8 "USER %s pid %3d cmd %s",
//                file->cf_User, (int)line->cl_Pid, line->cl_Shell);
//            if (line->cl_Pid < 0) {
//                file->cf_Ready = 1;
//            } else if (line->cl_Pid > 0) {
//                file->cf_Running = 1;
//            }
        }
//    }
}

/*
 * CheckJobs() - check for job completion
 *
 * Check for job completion, return number of jobs still running after
 * all done.
 */
//static int CheckJobs(void)
//{
//    CronFile *file;
//    CronLine *line;
//    int nStillRunning = 0;
//
//    for (file = FileBase; file; file = file->cf_Next) {
//        if (file->cf_Running) {
//            file->cf_Running = 0;
//
//            for (line = file->cf_LineBase; line; line = line->cl_Next) {
//                int status, r;
//                if (line->cl_Pid <= 0)
//                    continue;
//
//                r = waitpid(line->cl_Pid, &status, WNOHANG);
//                if (r < 0 || r == line->cl_Pid) {
//                    EndJob(file->cf_User, line);
//                    if (line->cl_Pid) {
//                        file->cf_Running = 1;
//                    }
//                } else if (r == 0) {
//                    file->cf_Running = 1;
//                }
//            }
//        }
//        nStillRunning += file->cf_Running;
//    }
//    return nStillRunning;
//}



/********************************************************
 * function:    cron_monitor
 * description: start to run cron parsing service
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int cron_monitor( void )
{
    static time_t t1 = 0;
    time_t t2;
    long dt;
    int rescan = 60;
    int sleep_time = 60;
    static uint8_t flag = 0;        // we will set it every minute
    CronLine *line = g_cronHead;
    if (t1 == 0)
    {
        t1 = time(NULL);
    }
    t2 = time(NULL);
    dt = t2%60;
//        crondlog("utc: %d,start: %d, time: %d", t2, g_cronStart, dt);
    if (g_cronStart == 0)
    {
        return kNoErr;
    }
    if (((sleep_time - time(NULL) % sleep_time) < 2)||(time(NULL) % sleep_time)<2){
//        for (;;) {
//            sleep((sleep_time + 1) - (time(NULL) % sleep_time));
//            crondlog("t2: %d, flag: %d", time(NULL), flag);
        if (flag)
        {
            return 0;
        }
        flag = 1;
        t2 = time(NULL);
        dt = (long)t2 - (long)t1;

        /*
         * The file 'cron.update' is checked to determine new cron
         * jobs.  The directory is rescanned once an hour to deal
         * with any screwups.
         *
         * check for disparity.  Disparities over an hour either way
         * result in resynchronization.  A reverse-indexed disparity
         * less then an hour causes us to effectively sleep until we
         * match the original time (i.e. no re-execution of jobs that
         * have just been run).  A forward-indexed disparity less then
         * an hour causes intermediate jobs to be run, but only once
         * in the worst case.
         *
         * when running jobs, the inequality used is greater but not
         * equal to t1, and less then or equal to t2.
         */

        if (dt < -60 * 60 || dt > 60 * 60) {
//                crondlog(WARN9 "time disparity of %d minutes detected", dt / 60);
        } else if (dt > 0) {
            mico_rtos_lock_mutex(&g_cron_mutex);

            TestJobs(t1, t2);
            RunJobs();
//                sleep(5);
//                if (CheckJobs() > 0) {
//                    sleep_time = 10;
//                } else {
//                    sleep_time = 60;
//                }
            mico_rtos_unlock_mutex(&g_cron_mutex);
        }
        t1 = t2;
    }
    else if (flag)
    {
        // reset flag value
        flag = 0;
    }

//    mico_rtos_lock_mutex(&g_cron_mutex);
//    line = g_cronHead;
//    while (line)
//    {
//        crondlog("line id: %s, day: %x, hour: %x, minitue: %x", line->id, *((uint32_t *)line->cl_Days), *((uint32_t *)line->cl_Hrs + 4), *((uint32_t *)line->cl_Mins));
//        line = line->cl_Next;
//    }
//    mico_rtos_unlock_mutex(&g_cron_mutex);
}


/********************************************************
 * function:    cron_start_service
 * description: start to run cron parsing service
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int cron_start_service( void )
{
    // init mutex to operate link table
    mico_rtos_init_mutex(&g_cron_mutex);
    g_cronStart = 1;
}

/********************************************************
 * function:    cron_stop_service
 * description: stop to run cron parsing service
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int cron_stop_service( void )
{
    g_cronStart = 0;
    mico_rtos_deinit_mutex(&g_cron_mutex);
}
