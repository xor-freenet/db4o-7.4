/*
  This code is part of FCPTools - an FCP-based client library for Freenet
	
  Designed and implemented by David McNab <david@rebirthing.co.nz>
  CopyLeft (c) 2001 by David McNab
	
	Currently maintained by Jay Oliveri <ilnero@gmx.net>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ezFCPlib.h"

#include <stdio.h>
#include <stdlib.h>

extern void _fcpSockDisconnect(hFCP *hfcp);


hFCP *_fcpCreateHFCP(void)
{
  hFCP *h;

	h = (hFCP *)malloc(sizeof (hFCP));
	memset(h, 0, sizeof (hFCP));

	h->host = malloc(strlen(_fcpHost) + 1);
	strcpy(h->host, _fcpHost);

	h->port = _fcpPort;
	h->htl = _fcpHtl;
	h->regress = _fcpRegress;
	h->rawmode = _fcpRawmode;

	_fcpLog(FCP_LOG_DEBUG, "using address %s:%d", h->host, h->port);

	return h;
}

void _fcpDestroyHFCP(hFCP *h)
{
	if (h) {
		if (h->socket < 0) _fcpSockDisconnect(h);

		if (h->host) free(h->host);
		if (h->description) free(h->description);
		if (h->key) _fcpDestroyHKey(h->key);

		free(h);
	}
}

hBlock *_fcpCreateHBlock(void)
{
	hBlock *h;

	h = (hBlock *)malloc(sizeof (hBlock));
	memset(h, 0, sizeof (hBlock));

	h->uri = _fcpCreateHURI();

	return h;
}

void _fcpDestroyHBlock(hBlock *h)
{
	if (h) {
		if (h->filename) free(h->filename);
		if (h->uri) _fcpDestroyHURI(h->uri);

		free(h);
	}
}

hKey *_fcpCreateHKey(void)
{
	hKey *h;

	h = (hKey *)malloc(sizeof (hKey));
	memset(h, 0, sizeof (hKey));

	h->uri        = _fcpCreateHURI();
	h->target_uri = _fcpCreateHURI();

	return h;
}

void _fcpDestroyHKey(hKey *h)
{
	if (h) {
		int i;

		if (h->uri)   _fcpDestroyHURI(h->uri);
		if (h->target_uri) _fcpDestroyHURI(h->target_uri);

		if (h->mimetype) free(h->mimetype);
		if (h->tmpblock) free(h->tmpblock);

		for (i=0; i < h->segment_count; _fcpDestroyHSegment(h->segments[i++]));

		free(h);
	}
}

hURI *_fcpCreateHURI(void)
{
	hURI *h;

	h = (hURI *)malloc(sizeof (hURI));
	memset(h, 0, sizeof (hURI));
	
	return h;
}

void _fcpDestroyHURI(hURI *h)
{
	if (h) {
		if (h->uri_str) free(h->uri_str);
		if (h->keyid) free(h->keyid);
		if (h->docname) free(h->docname);
		if (h->metastring) free(h->metastring);

		free(h);
	}
}

/*
	_fcpParseURI()

	This function parses a string containing a fully-qualified Freenet URI
	into simpler components.  It is written to be re-entrant on the same
	hURI pointer (it can be called repeatedly without being re-created.
*/
int _fcpParseURI(hURI *uri, char *key)
{
	int len;
	
	char *p;
	char *p2;

	char *p_key;

	p_key = key;

	/* clear out the dynamic arrays before attempting to parse a new uri */
	if (uri->uri_str) free(uri->uri_str);
  if (uri->keyid) free(uri->keyid);
	if (uri->docname) free(uri->docname);
	if (uri->metastring) free(uri->metastring);

	/* zero the block of memory */
	memset(uri, 0, sizeof (hURI));

  /* skip 'freenet:' */
  if (!strncmp(key, "freenet:", 8))
    key += 8;
	
  /* classify key header */
  if (!strncmp(key, "SSK@", 4)) {
		char *string_end;
		
    uri->type = KEY_TYPE_SSK;
		key += 4;

		/* Copy out the key id, up until the '/' char.*/
		for (p = key; *p != '/'; p++);
		len = p - key;

		uri->keyid = (char *)malloc(len + 1);

		strncpy(uri->keyid, key, len);
		p2 = uri->keyid + len;
		*p2 = 0;

		/* Make key point to the char after '/' */
		key = ++p;
		
		/* handle rest of key, depending on what's included and what's implied */
		
		/* if the '//' sequence isn't in the uri.. */
		if (!(string_end = strstr(p, "//"))) {
			
			uri->docname = strdup(p);

			uri->uri_str = malloc(strlen(uri->keyid) + strlen(uri->docname) + 20);
			sprintf(uri->uri_str, "freenet:SSK@%s/%s//", uri->keyid, uri->docname); 
		}
		else { /* there's a "//"; must use that as the next border */
			
			string_end = strstr(p, "//");
			len = string_end - p;
			
			uri->docname = strndup(p, len);
			
			/* set key to first char after "//" */
			key = string_end + 1;
			
			/* now set the remaining part to metastring */
			uri->metastring = strdup(key);
			
			uri->uri_str = malloc(strlen(uri->keyid) + strlen(uri->docname) + strlen(uri->metastring) + 20);
			sprintf(uri->uri_str, "freenet:SSK@%s/%s//%s", uri->keyid, uri->docname, uri->metastring); 
		}
  }

  else if (!strncmp(key, "CHK@", 4)) {
		
    uri->type = KEY_TYPE_CHK;
		key += 4;
    
		len = strlen(key);

		if (len) {
			uri->keyid = (char *)malloc(len + 1);
			strcpy(uri->keyid, key);
		}
		
		if (uri->keyid) {
			uri->uri_str = (char *)malloc(strlen(uri->keyid) + 13);
			sprintf(uri->uri_str, "freenet:CHK@%s", uri->keyid);
		}
		else {
			uri->uri_str = (char *)malloc(13);
			strcpy(uri->uri_str, "freenet:CHK@");
		}
  }
  
	/* freenet:KSK@freetext.html */

  else if (!strncmp(key, "KSK@", 4)) {

    uri->type = KEY_TYPE_KSK;

    key += 4;

		len = strlen(key);

		uri->keyid = (char *)malloc(len + 1);
		strcpy(uri->keyid, key);
		*(uri->keyid + len) = 0;

		uri->uri_str = (char *)malloc(strlen(uri->keyid) + 13);
		sprintf(uri->uri_str, "freenet:KSK@%s", uri->keyid);
  }
  
  else {
		_fcpLog(FCP_LOG_DEBUG, "error attempting to parse invalid key \"%s\"", p_key);
    return 1;
  }

  return 0;
}

hMetadata *_fcpCreateHMetadata(void)
{
	hMetadata *h;

	h = (hMetadata *)malloc(sizeof (hMetadata));
	memset(h, 0, sizeof (hMetadata));

	return h;
}

void _fcpDestroyHMetadata(hMetadata *h)
{
	if (h) {
	}

	return;
}


/*************************************************************************/
/* FEC specific */
/*************************************************************************/


hSegment *_fcpCreateHSegment(void)
{
  hSegment *h;

	h = (hSegment *)malloc(sizeof (hSegment));
  memset(h, 0, sizeof (hSegment));

  return h;
}

void _fcpDestroyHSegment(hSegment *h)
{
	if (h) {
		int i;

		if (h->header_str) free(h->header_str);

		if (h->db_count)
			for (i=0; i < h->db_count; _fcpDestroyHBlock(h->data_blocks[i++]));

		if (h->cb_count)
			for (i=0; i < h->cb_count; _fcpDestroyHBlock(h->check_blocks[i++]));

		free(h);
	}
}
