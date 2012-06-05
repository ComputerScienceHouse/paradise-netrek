/*
 * common.h
 */

#ifndef COMMON_H
#define COMMON_H

#define err_fatal(x) {err(x); exit(1);}

#define err_sys_fatal(x) {err_sys(x); exit(1);}

#define VERSSTR "1.1"

#endif /* COMMON_H */
