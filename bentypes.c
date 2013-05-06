#include "bdecode.h"

bd_list* bd_list_create()
{
	bd_list* list = malloc(sizeof(bd_list));
	// TODO: Nomem check hurr.

	list->entries = malloc(sizeof(bd_entry) * 2);
	list->allocated = 2;
	list->used = 0;
	return list;
}
void bd_list_add(bd_list* list, enum bd_type type, void* data)
{
	bd_entry* e;
	if(list->used == list->allocated)
	{
		list->allocated = list->allocated + (list->allocated / 2);
		e = realloc(list->entries, sizeof(bd_entry) * (list->allocated));
		list->entries = e;
	}

	list->entries[list->used].type = type;
	list->entries[list->used].data = data;
	list->used++;
}
void bd_list_destroy(bd_list* list)
{
	int i;
	for(i = 0; i < list->used; i++)
	{
		switch(list->entries[i].type)
		{
			case DICTIONARY:
				bd_dict_destroy(list->entries[i].dict);
				break;
			case LIST:
				bd_list_destroy(list->entries[i].list);
				break;
			case STRING:
				free(list->entries[i].str);
				break;
			default:
				break;
		}
	}
	free(list->entries);
	free(list);
}

void bd_dict_add(bd_dict** dict, char* key, enum bd_type type, void* data)
{
	bd_dict* kvp = malloc(sizeof(bd_dict));

	// TODO: Nomem check hurr.

	kvp->key = key;
	kvp->data = data;
	kvp->type = type;
	kvp->next = *dict;
	*dict = kvp;
}
bd_entry* bd_dict_find(bd_dict* dict, char* key)
{
	bd_dict* iter = dict;
	while(iter)
	{
		if(strcmp(iter->key, key) == 0)
		{
			return iter;
		}
		iter = iter->next;
	}
	return 0;
}
void bd_dict_destroy(bd_dict* dict)
{
	bd_dict* dt;
	bd_dict* d = dict;

	while(d)
	{
		dt = d->next;
		switch(d->type)
		{
			case DICTIONARY:
				bd_dict_destroy(d->dict);
				break;
			case LIST:
				bd_list_destroy(d->list);
				break;
			case STRING:
				free(d->str);
				break;
			default:
				break;
		}
		free(d);
		d = dt;
	}
}

void bd_dict_print(bd_dict* dict, int indent)
{
	indent = (indent > 32) ? 0 : indent;
	bd_dict* itr = dict;
	while(itr)
	{
		printf("%*s" "%s::\n", indent, "  ", itr->key);
		switch(itr->type)
		{
			case DICTIONARY:
				bd_dict_print(itr->data, indent + 1);
				break;
			case STRING:
				printf("%*s" "%s\n", indent + 1, "  ", itr->str);
				break;
			case NUMBER:
				printf("%*s" "%lli\n", indent + 1, "  ", (long long)itr->data);
				break;
			case LIST:
				bd_list_print(itr->data, indent + 1);
				break;
			default:
				break;
		}
		itr = itr->next;
	}
}
void bd_list_print(bd_list* list, int indent)
{
	indent = (indent > 32) ? 0 : indent;
	int i;
	for(i = 0; i < list->used; i++)
	{
		switch(list->entries[i].type)
		{
			case DICTIONARY:
				bd_dict_print(list->entries[i].dict, indent);
				break;
			case NUMBER:
				printf("%*s" "%lli\n", indent, "  ", (long long)list->entries[i].data);
				break;
			case LIST:
				bd_list_print(list->entries[i].list, indent + 1);
				break;
			case STRING:
				printf("%*s" "%s\n", indent, "  ", list->entries[i].str);
				break;
			default:
				break;
		}
	}
}

