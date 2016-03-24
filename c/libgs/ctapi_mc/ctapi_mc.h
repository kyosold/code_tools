//
//  ctapi_mc.h
//  
//
//  Created by SongJian on 15/12/8.
//
//

#ifndef ctapi_mc_h
#define ctapi_mc_h

int set_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key, char *mc_value, time_t expiration);

char *get_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key, int *ret);

int delete_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key);

#endif /* ctapi_mc_h */
