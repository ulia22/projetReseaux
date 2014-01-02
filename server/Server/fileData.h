/* 
 * File:   fileData.h
 * Author: ulia22
 *
 * Created on 26 décembre 2013, 19:30
 */

#ifndef FILEDATA_H
#define	FILEDATA_H

#ifdef	__cplusplus
extern "C" {
#endif
int initFileData();
int getNewClePair(void);
int sendMetaFile(int sdClient, char *name, char *prefix);

#ifdef	__cplusplus
}
#endif

#endif	/* FILEDATA_H */

