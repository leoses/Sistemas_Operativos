#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
	int bytesRead = 0;
	int characterToCopy;
	int copyResult;

	//Si alguno de los ficheros no existe lanzamos fallo
	if(origin == NULL || destination == NULL) return -1;

	while((bytesRead < nBytes) && (characterToCopy = fgetc(origin))!= EOF ){

		//putc devuelve eof si ha ocurrido algun error
		if((copyResult= fputc(characterToCopy, destination)) == EOF) return -1;
		bytesRead++;
	}

	return bytesRead;
	
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char*
loadstr(FILE * file)
{
	//Si hay fallo en el archivo error
	if(file == NULL) return NULL;

	//Ahora averiguamos el tamaño de la cadena que queremos almacenar
	//Leemos caracter a caracter hasta encontrar \0 (fin de cadena) o 
	//hasta que encontremos un error

	size_t tamano = 0;
	char caracter;

	while((caracter = getc(file)) != '\0')
	{
		if(caracter == EOF) return NULL;
		tamano++;
	}

	char* cadena;
	//Reservamos memoria para la cadena de texto
	//+1 para formar correctamente la cadena, lo que corresponde
	//al \0
	cadena = malloc(tamano + 1); 

	//Retrocedemos en la lectura del archivo el numero de elementos que hemos
	//conseguido leer por el tamaño de esos elementos en bytes
	int result = fseek(file, -((tamano+1)*sizeof(char)), SEEK_CUR);
	if(result == -1) return NULL;

	size_t elementosCargados = fread(cadena, sizeof(char),tamano + 1,file);

	//fread devuelve, en este caso, el numero de chars que hemos conseguido leer
	//Si ese numero es mas pequeño que el tamaños de la
	//cadena es que ha habido un error
	if(elementosCargados < tamano) return NULL;
	else return cadena;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry*
readHeader(FILE * tarFile, int *nFiles)
{
	//Comprobamos que el fichero sea valido
	if (tarFile == NULL)return NULL;

	//Leemos el numero de ficheros que hay en el tar puesto 
	//que sabemos que el primer elemento del archivo es un int
	//con el numero de ficheros
	size_t result = fread(nFiles, sizeof(int), 1, tarFile);
	//Comprobamos que la lectura ha sido correcta
	if(result != 1) return NULL;

	//Reservamos el espacio de memoria que tendrá la cabecera del tar
	stHeaderEntry* header = malloc(sizeof(stHeaderEntry)*(*nFiles));
	//Comprobamos :)
	if(header == NULL) return NULL;

	//Para cada fichero leemos su nombre usando el metodo loadstr
	for(int  i = 0; (i <(*nFiles)); i++){
		char* name = loadstr(tarFile);
		//Comprobamos que se ha leido correctamente
		if(name == NULL)return NULL;
		//Reservamos memoria para el nombre
		header[i].name = malloc(strlen(name));
		//Comprobamos :)
		if(header[i].name ==NULL)return NULL;
		//Copiamos el nombre que hemos obtenido en la direccion del header correspondiente
		strcpy(header[i].name, name);
		
		//Leemos el entero sin signo que representa el tamano del fichero
		result = fread(&header[i].size, sizeof(unsigned int), 1,tarFile);
		//Comprobamos :)
		if(result != 1) return NULL;
	}

	//Si llegamos al final, es que todo esta correcto
	return header;
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int
createTar(int nFiles, char *fileNames[], char tarName[])
{
	 //Se abre el fichero mtar
    //Creamos un puntero a un puntero de tipo FILE para crear el mtar
    FILE* mtar= fopen(tarName,"w");

	if(mtar == NULL) return EXIT_FAILURE;

	stHeaderEntry* header = malloc(sizeof(stHeaderEntry)*nFiles); //TO DO: DESALOJAR
	if(header == NULL)return EXIT_FAILURE;

	//Calculamos cuanto espacio vamos a necesitar para el header de nuestro tarball
	size_t headerSize = 0;
	headerSize += sizeof(int); //entero con el numero de ficheros

	for(int i = 0; (i< nFiles); i++){
		headerSize += sizeof(unsigned int ); //entero con el numero de bytes de cada fichero
		headerSize += strlen(fileNames[i]) + 1; //longitud del nombre del fichero +1 por \0
	}

	//Ahora nos saltamos el header para copiar el contenido de los archivos
	fseek(mtar, headerSize, SEEK_SET);

	//Copiamos el contenido de los ficheros
	for(int i = 0; (i<nFiles); i++){
		FILE* archivo = fopen(fileNames[i],"r");
		if(archivo == NULL) return EXIT_FAILURE;

		int archiveBytes = copynFile(archivo, mtar, INT_MAX);
		if(archiveBytes < 0) return EXIT_FAILURE;

		header[i].size = archiveBytes;
		
		header[i].name = malloc(strlen(fileNames[i])+1); //TO DO: DESALOJAR
		if(header[i].name == NULL)return EXIT_FAILURE;
		strcpy(header[i].name,fileNames[i]);

		fclose(archivo);
	}

	//Nos movemos de nuevo al principio del fichero
	fseek(mtar, 0L, SEEK_SET);

	//Escribimos el numero de ficheros con los que cuenta nuestro tar
	if(fwrite(&nFiles, sizeof(int), 1, mtar) != 1)return EXIT_FAILURE;

	//Escribimos las cabeceras para cada archivo
	for(int  i = 0; (i < nFiles); i++){
		if(fwrite(header[i].name, strlen(fileNames[i])+1, 1,mtar) != 1)return EXIT_FAILURE; //Nombre
		if(fwrite(&header[i].size, sizeof(unsigned int), 1, mtar)!=1)return EXIT_FAILURE;//tamano bytes
	}

	//Liberar memoria reservada, primero de los strings de los nombres de los
	//ficheros y despues del puntero a los distintos headers
	for(int i = 0; i< nFiles; i++){
		free(header[i].name);
	}

	free(header);

	//Fichero rellenado, lo cerramos
	fclose(mtar);

	

	//Si hemos llegado hasta aqui, singnifica que todo 
	//ha salido bien
	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int
extractTar(char tarName[])
{
	// Complete the function
	FILE* tarFile= fopen(tarName,"r");

	int nFiles = 0;
	stHeaderEntry* header = readHeader(tarFile, &nFiles);
	if(header == NULL)return EXIT_FAILURE;

	for(int i = 0; (i< nFiles); i++){
		//char message[] = header[i].name;
		FILE* archivo = fopen(header[i].name, "w");
		if(archivo == NULL) return EXIT_FAILURE;

		//int bytesCopiados = copynFile(tarFile,archivo, header[i].size);
		//if(bytesCopiados != header[i].size)return EXIT_FAILURE;
		
		char* buffer = malloc(sizeof(char)* header[i].size);

		size_t bytesLeidos = fread(buffer, sizeof(char), header[i].size, tarFile);
		if(bytesLeidos != header[i].size) return EXIT_FAILURE;
		
		size_t bytesEscritos = fwrite(buffer,sizeof(char), header[i].size,archivo);
		if(bytesEscritos != header[i].size) return EXIT_FAILURE;

		free(buffer);
		fclose(archivo);
	}

	fclose(tarFile);

	return EXIT_SUCCESS;
}
