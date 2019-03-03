#include <gelf.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function, that iterates over every section of ELF and returns index of section with
// its name being "name" given in second parameter
size_t get_scn_index(Elf *elf_file, const char* name)
{
	Elf_Scn *section = NULL;
	size_t shstrndx;
	char *section_name;
	GElf_Shdr shdr;
	
	//get the index of section with section name strings from the ELF header
	if(elf_getshdrstrndx(elf_file, &shstrndx) != 0)
	{
		fprintf(stderr,"Could not retrieve shstr index: %s\n", elf_errmsg(-1));
		return -1;
	}

	while((section = elf_nextscn(elf_file, section)) != NULL)
	{
		if(gelf_getshdr(section, &shdr) != &shdr)
		{
			fprintf(stderr,
				"Could not get the section header\n", elf_errmsg(-1));
			return -1;
		}

		section_name = elf_strptr(elf_file, shstrndx, shdr.sh_name);
		if(section_name == NULL)
		{
			fprintf(stderr,
				"Could not get the section name" ,elf_errmsg(-1));
			return -1;
		}
		

		if(!strcmp(name, section_name))
			return elf_ndxscn(section);
	}
	return -2;
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Wrong arg count\n");
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "Could not open file %s\n", argv[1]);
		return 2;
	}

	if(elf_version(1) == EV_NONE)
	{
		fprintf(stderr, "Incompatible elf versions");
		return 3;
	}

	Elf *elf_file = elf_begin(fd, ELF_C_READ_MMAP, NULL);
	if(elf_file == NULL)
	{
		fprintf(stderr, "Could not interpret the file as elf.\n");
		return 4;
	}
	
	//this program only works with ELF files, not with ar archives or anything else
	int kind = elf_kind(elf_file);
	if(kind != 3)
	{
		
		fprintf(stderr, "The file %s isn't an ELF file\n", argv[1]);
		return 5;
	}

	size_t str_index = get_scn_index(elf_file, ".strtab");
	if(str_index < 0)
	{
		fprintf(stderr, "Could not get the index of the string table\n");
		return 6;
	}

	size_t sym_index = get_scn_index(elf_file, ".symtab");
	if(sym_index < 0)
	{
		fprintf(stderr, "Could not get the index of the symbol table\n");
		return 7;
	}

	Elf_Scn *section = elf_getscn(elf_file, sym_index);
	if(section == NULL)
	{
		fprintf(stderr, "Could not get the symbol section: %s\n",
				elf_errmsg(-1));
		return 8;
	}
	
	GElf_Sym sym;
	char *name;

	//get the symbol table data
	Elf_Data *data = NULL;
	data = elf_getdata(section, data);

	printf("value    bind type size     name\n");
	//main loop, iterates over every symbol and prints info about it
	for(int i = 0; gelf_getsym(data, i, &sym) != NULL; i++)
	{
		name = elf_strptr(elf_file, str_index, sym.st_name);
		if(name == NULL)
		{
			fprintf(stderr,"Could not retrieve the name of a symbol:%s\n",
					elf_errmsg(-1));
			return 9;
		}
		if(*name == 0)
			continue;

		printf("%8.8d %-4d %-4d %-8d %s\n",
				sym.st_value,
				GELF_ST_BIND(sym.st_info),
				GELF_ST_TYPE(sym.st_info),
				sym.st_size,
				name);
	}

	//cleanup
	elf_end(elf_file);
	close(fd);
	
	return 0;
}
