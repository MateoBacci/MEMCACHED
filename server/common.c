#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

const char * code_to_str(enum code e)
{
  switch (e) {
  case PUT:	return "PUT";
  case GET:	return "GET";
  case DEL:	return "DEL";
  case STATS:	return "STATS";
  case OK: return "OK\n";
  case EINVALID: return "EINVAL\n";
  case ENOTFOUND:	return "ENOTFOUND\n";
  case EBINARY:	return "EBINARY\n";
  case EBIG:	return "EBIG\n";
  case EUNK:	return "EUNK\n";
  default:
    assert(0);
    return "";
  }
}
