/*
 * Copyright (c) 1997 Peter Wemm.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the FreeBSD Project
 *	by Peter Wemm.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * so there!
 *
 * $Id: ifconfig.h,v 1.3 1997/05/10 17:14:53 peter Exp $
 */

extern struct ifreq ifr;

extern char name[32];	/* name of interface */
extern int allmedia;
struct afswtch;

extern void setmedia(const char *, int, int, const struct afswtch *rafp);
extern void setmediaopt(const char *, int, int, const struct afswtch *rafp);
extern void unsetmediaopt(const char *, int, int, const struct afswtch *rafp);
extern void media_status(int s, struct rt_addrinfo *);

extern void setvlantag(const char *, int, int, const struct afswtch *rafp);
extern void setvlandev(const char *, int, int, const struct afswtch *rafp);
extern void unsetvlandev(const char *, int, int, const struct afswtch *rafp);
extern void vlan_status(int s, struct rt_addrinfo *);
