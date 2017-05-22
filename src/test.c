#include <stdio.h>

#include "logger.h"
#include "yada.h"
#include "item.h"
#include "seekrange.h"
#include "md5utils.h"

#define TEST_RANGES 0
#define TEST_CDS 1
#define TEST_YADA 0


void main()
{
   int x;

#if TEST_RANGES == 1
   npt_time t;
   bytes_range b;
   timeseek_range r;
   char *p;

   // OK
   x = npt_parse( "*", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = npt_parse( "now", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = npt_parse( "310", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // NOK
   x = npt_parse( "310.", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = npt_parse( "310.1\r\n", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = npt_parse( "1:12:23", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // NOK
   x = npt_parse( "1:62:32", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // NOK
   x = npt_parse( "1:62:32.", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // NOK
   x = npt_parse( "/1:62:32.1236677", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = npt_parse( "1:02:32.1236677", &t );
   if( x )
   {
      p = npt_tostring( &t, NULL, 0 );
      printf( "npt: %s\n", p );
      free( p );
   }

   // OK
   x = bytesrange_parse( "bytes=12345678-\r", &b );
   if( x )
   {
      p = bytesrange_tostring( &b, NULL, 0 );
      printf( "bytesrange: %s\n", p );
      free( p );
   }

   // NOK
   x = bytesrange_parse( "bytes=12345678\r", &b );
   if( x )
   {
      p = bytesrange_tostring( &b, NULL, 0 );
      printf( "bytesrange: %s\n", p );
      free( p );
   }

   // OK
   x = bytesrange_parse( "bytes=12345678-1222333444\r", &b );
   if( x )
   {
      p = bytesrange_tostring( &b, NULL, 0 );
      printf( "bytesrange: %s\n", p );
      free( p );
   }

   // OK
   x = timeseek_parse( "npt=310.1-1:02:32.123\r", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // NOK
   x = timeseek_parse( "npt=310.1-1:02:32.123/", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // OK
   x = timeseek_parse( "npt=310.1-/55555.2", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // NOK
   x = timeseek_parse( "npt=310.1- bytes=55555", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // NOK
   x = timeseek_parse( "npt=310.1- bytes=55555-", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // NOK
   x = timeseek_parse( "npt=310.1 bytes=55555-666666", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // NOK
   x = timeseek_parse( "npt=310.1- bytes=55555-666666", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

   // OK
   x = timeseek_parse( "npt=310.1-420.2/* bytes=55555-666666/*", &r );
   if( x )
   {
      p = timeseek_tostring( &r, NULL, 0 );
      printf( "timeseek: %s\n", p );
      free( p );
   }

#endif

#if TEST_CDS == 1

   cds_test();

#endif

#if TEST_YADA == 1
   item_info *ii;

   logger_set_log_level( LOG_TRACE );

   if( yada_init("D:\\MyData\\20 Personal\\Development\\C++\\dlnacpp\\dlnacpp\\config.xml") != 0 )
   {
      scanf( "%d", &x );
      return;
   }

   yada_shutdown();

#endif

   x = -1;
   while ( x != 0 )
   {
      printf( "Please enter an option:\n\t0 - terminate\n\t: " );
      scanf( "%d", &x );
   }


   scanf( "%d", &x );

}