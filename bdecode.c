#include <ctype.h>
#include <math.h>

#include "bdecode.h"

// Main entry point for decoding bencoded .torrent files. 
void* decode(unsigned char* buf, size_t size)
{
  int index = 0;

  if(buf[0] != 'd')
  {
	  printf("Invalid character :%s: at start of bencoded section.", (char)buf[0]);
	  return 1;
  }
  return decode_dictionary(buf, &index, size);
}

bd_dict* decode_dictionary(unsigned char* buf, int* index, size_t size)
{
  char c;
  bd_dict* retdict;
  int parse_key;
  char* cur_key;
  void* cur_val;
  
  (*index)++;
  parse_key = 1;
  retdict = (bd_dict*)malloc(sizeof(bd_dict));
  while(buf[*index] != 'e')
  {
    c = buf[*index];
    if(parse_key)
    {
      if(isdigit(c))
      {
        cur_key = decode_string(buf, index, size);
        parse_key = 0;
      }
      else
      {
        printf("Invalid key format for dictionary.");
      }
    }
    else
    {
      if(isdigit(c))
      {
        char* str;
        str = decode_string(buf, index, size);
        bd_dict_add(&retdict, cur_key, STRING, str);
      }
      else
      {
        switch(c)
        {
          case 'i':
            cur_val = decode_number(buf, index, size);
            bd_dict_add(&retdict, cur_key, NUMBER, cur_val);
            break;
          case 'l':
            cur_val = decode_list(buf, index, size);
            bd_dict_add(&retdict, cur_key, LIST, cur_val);
            break;
          case 'd':
            cur_val = decode_dictionary(buf,index, size);
            bd_dict_add(&retdict, cur_key, DICTIONARY, cur_val);
            break;
          default:
            printf("Invalid data type specifier.");
            return 1;
        }
      }
      parse_key = 1;
    }
    (*index)++;
  }
  return retdict;
}

// Decodes a bencoded number.
//
// PRECONDITION
// Given a char buffer of size |size| with index set to the offset
// where the type identifier appears.
//
// RETURNS
// The number described by the bencoded range. Note that this may
// be positive, negative or zero.
//
// POSTCONDITION
// Index will be set to the terminating character of this range, in
// this case the first occurrence of 'e' in the bencoded block.
long long decode_number(unsigned char* buf, int* index, size_t size)
{
	long long retnum;
	char* nstart;
	char* nend;

	(*index)++;
	nstart = &buf[(*index)];
	nend = &buf[(*index) + 1];
	retnum = strtoll(nstart, &nend, 10);
	(*index) += (nend - nstart);
	return retnum;
}

// Decodes a bencoded byte string.
//
// PRECONDITION
// Given a char buffer of size |size| with index set to the offset
// where the type identifier appears.
//
// RETURNS
// Reference to a null-terminated string. Note that this string is
// guaranteed to be null-terminated but is not guaranteed to be valid
// ASCII or UTF-8 encoded. It's just bytes, bro.
//
// POSTCONDITION
// Index will be set to the offset where the terminating character
// for this parsed block occurred. In this case, it will be set to the
// index of the last byte in the string as it appears in the buffer.
char* decode_string(unsigned char* buf, int* index, size_t size)
{
	int len;
	char* retstr;

	len = atoi(&buf[*index]);
	(*index) += log10(len) + 2;
	retstr = malloc((sizeof(char) * len) + 1);

	memcpy(retstr, (const void*)&buf[*index], sizeof(char) * len);
	retstr[len] = '\0';

	(*index) += (len - 1);
	return retstr;
}

// Decodes a bencoded list of elements.
//
// PRECONDITION
// Given a char buffer of size |size| with index set to the offset
// where the type identifier 'l' appears.
//
// RETURNS
// Reference to a bd_list object containing the data described in
// the bencoded block. Note that it is possible to nest these
// container types.
//
// POSTCONDITION
// Index will be set to the offset where the terminating character
// 'e' was found.
bd_list* decode_list(unsigned char* buf, int* index, size_t size)
{
	char c;
	void* cur_ent;
	bd_list* list = bd_list_create();

	(*index)++;
	while(buf[*index] != 'e')
	{
		c = buf[*index];
		switch(c)
		{
			case 'i':
				cur_ent = decode_number(buf, index, size);
				bd_list_add(list, NUMBER, cur_ent);
				break;
			case 'l':
				cur_ent = decode_list(buf, index, size);
				bd_list_add(list, LIST, cur_ent);
				break;
			case 'd':
				cur_ent = decode_dictionary(buf, index, size);
				bd_list_add(list, DICTIONARY, cur_ent);
				break;
			default:
				if(isdigit(c))
				{
					cur_ent = decode_string(buf, index, size);
					bd_list_add(list, STRING, cur_ent);
				}
				break;
		}
		(*index)++;
	}
	return list;
}
