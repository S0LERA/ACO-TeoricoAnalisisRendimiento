/* Pract2  RAP 09/10    Javier Ayllon*/
/*Código modificado por Juan Mena*/

#include <openmpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h> 
#include <assert.h>   
#include <unistd.h>   
#define NIL (0)  
#define NUM_HIJOS 7
#define FILTRO 0
#define MIN(a, b) (((a) < (b)) ? (a) : (b))   

/*Variables Globales */

XColor colorX;
Colormap mapacolor;
char cadenaColor[]="#000000";
Display *dpy;
Window w;
GC gc;

/*Funciones auxiliares */

void initX() {

      dpy = XOpenDisplay(NIL);
      assert(dpy);

      int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
      int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

      w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                     400, 400, 0, blackColor, blackColor);
      XSelectInput(dpy, w, StructureNotifyMask);
      XMapWindow(dpy, w);
      gc = XCreateGC(dpy, w, 0, NIL);
      XSetForeground(dpy, gc, whiteColor);
      for(;;) {
            XEvent e;
            XNextEvent(dpy, &e);
            if (e.type == MapNotify)
                  break;
      }

      mapacolor = DefaultColormap(dpy, 0);

}

void dibujaPunto(int x,int y, int r, int g, int b) {

        sprintf(cadenaColor,"#%.2X%.2X%.2X",r,g,b);
        XParseColor(dpy, mapacolor, cadenaColor, &colorX);
        XAllocColor(dpy, mapacolor, &colorX);
        XSetForeground(dpy, gc, colorX.pixel);
        XDrawPoint(dpy, w, gc,x,y);
        XFlush(dpy);

}

/* Programa principal */

int main (int argc, char *argv[]) {

  int rank,size;
  MPI_Comm commPadre;
  int tag;
  MPI_Status status;
  int buf[5];
  int errcodes[NUM_HIJOS];
  int seguir = 1;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_get_parent( &commPadre );

  /*Compruebo si hay procesos para realizar el renderizado*/
  if(NUM_HIJOS == 0){
  	seguir = 0;
  	fprintf(stderr, "ERROR, debe haber al menos 1 proceso hijo para el renderizado\n");
  }

  if ( (commPadre==MPI_COMM_NULL)
        && (rank==0) )  {
  	if(seguir != 0){
	initX();
	/* Codigo del maestro */

	/*En algun momento dibujamos puntos en la ventana algo como
	dibujaPunto(x,y,r,g,b);  */
  	MPI_Comm_spawn("renDistMejora", MPI_ARGV_NULL, NUM_HIJOS, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &commPadre, errcodes);

  	for(int i=0;i<400*400;i++){
    	MPI_Recv(&buf, 5, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, commPadre, MPI_STATUS_IGNORE);

    	dibujaPunto(buf[0],buf[1],buf[2],buf[3],buf[4]);
  	}
    	sleep(3);
    }
  	} else {
  		if(seguir != 0){
    	/* Codigo de todos los trabajadores */
    	/* El archivo sobre el que debemos trabajar es foto.dat */

    	int filasPorProceso=400/NUM_HIJOS;
    	int elementosPorProceso=filasPorProceso*400*sizeof(unsigned char)*3;

    	int inicioLectura=filasPorProceso*rank;
    	int finalLectura=inicioLectura+filasPorProceso;

    	MPI_File archivo;

    	/*Abrimos el fichero con permisos de lectura*/
    	MPI_File_open(MPI_COMM_WORLD, "foto.dat", MPI_MODE_RDONLY, MPI_INFO_NULL, &archivo);

    	MPI_File_set_view(archivo, elementosPorProceso*rank, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, "native", MPI_INFO_NULL);

    	/*Array para guardar los valores de RGB*/
    	unsigned char rgb[3];

    	/*Se controla el tamaño del último proceso*/
    	if(rank == NUM_HIJOS-1)
      		finalLectura=400;

    	for(int i=inicioLectura;i<finalLectura;i++){
      		for(int j=0;j<400;j++){
        		MPI_File_read(archivo, rgb, 3, MPI_UNSIGNED_CHAR, &status);
        		buf[0]=j;	/*Coordenada x*/
        		buf[1]=i;	/*Coordenada y*/

        		switch(FILTRO){
          			case 0: /*Sin filtro*/
            			buf[2]=(int)rgb[0]; /*Rojo*/
            			buf[3]=(int)rgb[1]; /*Verde*/
            			buf[4]=(int)rgb[2]; /*Azul*/
            		break;

          			case 1: /*Filtro negativo*/
            			buf[2]=255 - (int)rgb[0]; /*Rojo*/
            			buf[3]=255 - (int)rgb[1]; /*Verde*/
            			buf[4]=255 - (int)rgb[2]; /*Azul*/
            		break;

         			case 2: /*Filtro blanco y negro*/
            			buf[2]=((int)rgb[0] + (int)rgb[1] + (int)rgb[2])/3; /*Rojo*/
            			buf[3]=((int)rgb[0] + (int)rgb[1] + (int)rgb[2])/3; /*Verde*/
            			buf[4]=((int)rgb[0] + (int)rgb[1] + (int)rgb[2])/3; /*Azul*/
            		break;

          			case 3: /*Filtro sepia*/
            			buf[2]=MIN(((.3 * rgb[0] + .6 * rgb[1] + .1 * rgb[2]) + 40),255);	/*Rojo*/
            			buf[3]=MIN(((.3 * rgb[0] + .6 * rgb[1] + .1 * rgb[2]) + 15),255);	/*Verde*/
            			buf[4]=.3 * rgb[0] + .6 * rgb[1] + .1 * rgb[2];						/*Azul*/
            		break;
        		}        
        		MPI_Bsend(&buf, 5, MPI_INT, 0, 0, commPadre);
      		}
    	}
    	MPI_File_close(&archivo);
  	}
  }
  	MPI_Finalize();
  	return 0;
}