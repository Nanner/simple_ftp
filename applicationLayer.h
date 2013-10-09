#ifndef _APPLICATION_LAYER_H
#define _APPLICATION_LAYER_H

typedef struct {
       int fileDescriptor;
       int status;
} ApplicationLayer;

extern ApplicationLayer applicationLayerConf;

#endif