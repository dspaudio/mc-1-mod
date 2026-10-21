/* SPDX-License-Identifier: GPL-3.0-only */
/* 단일 진입점으로 컴파일하며 오디션 구현은 같은 TU에 포함한다. */
#include "auditions.c"

int main(int argc,char **argv) {
    if(argc!=3) {
        fprintf(stderr,"사용법: %s outputAudioDirectory reportJSON\n",argv[0]);
        return 1;
    }
    return auditions(argv[1],argv[2]);
}
