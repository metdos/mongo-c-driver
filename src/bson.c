/* bson.c */

#include "bson.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

const int initialBufferSize = 128;


/* ----------------------------
   READING
   ------------------------------ */

bson * bson_empty(bson * obj){
    static char * data = "\005\0\0\0\0";
    return bson_init(obj, data, 0);
}

bson * bson_init( bson * b , char * data , int mine ){
    b->data = data;
    b->owned = mine;
    return b;
}
int bson_size( bson * b ){
    int i;
    if ( ! b || ! b->data )
        return 0;
    bson_little_endian32(&i, b->data);
    return i;
}
void bson_destroy( bson * b ){
    if ( b->owned && b->data )
        free( b->data );
    b->data = 0;
    b->owned = 0;
}

static char hexbyte(char hex){
    switch (hex){
        case '0': return 0x0;
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0x4;
        case '5': return 0x5;
        case '6': return 0x6;
        case '7': return 0x7;
        case '8': return 0x8;
        case '9': return 0x9;
        case 'a': 
        case 'A': return 0xa;
        case 'b':
        case 'B': return 0xb;
        case 'c':
        case 'C': return 0xc;
        case 'd':
        case 'D': return 0xd;
        case 'e':
        case 'E': return 0xe;
        case 'f':
        case 'F': return 0xf;
        default: return 0x0; /* something smarter? */
    }
}

void bson_oid_from_string(bson_oid_t* oid, const char* str){
    int i;
    for (i=0; i<12; i++){
        oid->bytes[i] = (hexbyte(str[2*i]) << 4) | hexbyte(str[2*i + 1]);
    }
}
void bson_oid_to_string(const bson_oid_t* oid, char* str){
    static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i;
    for (i=0; i<12; i++){
        str[2*i]     = hex[(oid->bytes[i] & 0xf0) >> 4];
        str[2*i + 1] = hex[ oid->bytes[i] & 0x0f      ];
    }
    str[24] = '\0';
}
void bson_oid_gen(bson_oid_t* oid){
    static int incr = 0;
    static int fuzz = 0;
    int i = incr++; /*TODO make atomic*/
    int t = time(NULL);

    /* TODO rand sucks. find something better */
    if (!fuzz) fuzz = rand();
    
    bson_big_endian32(&oid->ints[0], &t);
    oid->ints[1] = fuzz;
    bson_big_endian32(&oid->ints[2], &i);
}

void bson_print( bson * b ){
    bson_print_raw( b->data , 0 );
}

void bson_print_raw( const char * data , int depth ){
    bson_iterator i;
    const char * key;
    int temp;
    char oidhex[25];
    bson_iterator_init( &i , data );

    while ( bson_iterator_next( &i ) ){
        bson_type t = bson_iterator_type( &i );
        if ( t == 0 )
            break;
        key = bson_iterator_key( &i );
        
        for ( temp=0; temp<=depth; temp++ )
            printf( "\t" );
        printf( "%s : %d \t " , key , t );
        switch ( t ){
        case bson_int: printf( "%d" , bson_iterator_int( &i ) ); break;
        case bson_double: printf( "%f" , bson_iterator_double( &i ) ); break;
        case bson_bool: printf( "%s" , bson_iterator_bool( &i ) ? "true" : "false" ); break;
        case bson_string: printf( "%s" , bson_iterator_string( &i ) ); break;
        case bson_null: printf( "null" ); break;
        case bson_oid: bson_oid_to_string(bson_iterator_oid(&i), oidhex); printf( "%s" , oidhex ); break;
        case bson_object:
        case bson_array:
            printf( "\n" );
            bson_print_raw( bson_iterator_value( &i ) , depth + 1 );
            break;
        default:
            fprintf( stderr , "can't print type : %d\n" , t );
        }
        printf( "\n" );
    }
}

/* ----------------------------
   ITERATOR
   ------------------------------ */

void bson_iterator_init( bson_iterator * i , const char * bson ){
    i->cur = bson + 4;
    i->first = 1;
}

int bson_iterator_more( bson_iterator * i ){
    return *(i->cur);
}

bson_type bson_iterator_next( bson_iterator * i ){
    int ds;

    if ( i->first ){
        i->first = 0;
        return (bson_type)(*i->cur);
    }
    
    switch ( bson_iterator_type(i) ){
    case bson_double: ds = 8; break;
    case bson_bool: ds = 1; break;
    case bson_null: ds = 0; break;
    case bson_int: ds = 4; break;
    case bson_long: ds = 8; break;
    case bson_oid: ds = 12; break;
    case bson_string: ds = 4 + bson_iterator_int_raw(i); break;
    case bson_object: ds = bson_iterator_int_raw(i); break;
    case bson_array: ds = bson_iterator_int_raw(i); break;
    default: 
        fprintf( stderr , "WTF: %d\n"  , (int)(i->cur[0]) );
        exit(-1);
        return 0;
    }
    
    i->cur += 1 + strlen( i->cur + 1 ) + 1 + ds;

    return (bson_type)(*i->cur);
}

bson_type bson_iterator_type( bson_iterator * i ){
    return (bson_type)i->cur[0];
}
const char * bson_iterator_key( bson_iterator * i ){
    return i->cur + 1;
}
const char * bson_iterator_value( bson_iterator * i ){
    const char * t = i->cur + 1;
    t += strlen( t ) + 1;
    return t;
}

/* types */

int bson_iterator_int_raw( bson_iterator * i ){
    int out;
    bson_little_endian32(&out, bson_iterator_value( i ));
    return out;
}
double bson_iterator_double_raw( bson_iterator * i ){
    double out;
    bson_little_endian64(&out, bson_iterator_value( i ));
    return out;
}
int64_t bson_iterator_long_raw( bson_iterator * i ){
    int64_t out;
    bson_little_endian64(&out, bson_iterator_value( i ));
    return out;
}

bson_bool_t bson_iterator_bool_raw( bson_iterator * i ){
    return bson_iterator_value( i )[0];
}

bson_oid_t * bson_iterator_oid( bson_iterator * i ){
    return (bson_oid_t*)bson_iterator_value(i);
}

int bson_iterator_int( bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}
double bson_iterator_double( bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}
int64_t bson_iterator_long( bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_int: return bson_iterator_int_raw(i);
        case bson_long: return bson_iterator_long_raw(i);
        case bson_double: return bson_iterator_double_raw(i);
        default: return 0;
    }
}

bson_bool_t bson_iterator_bool( bson_iterator * i ){
    switch (bson_iterator_type(i)){
        case bson_bool: return bson_iterator_bool_raw(i);
        case bson_int: return bson_iterator_int_raw(i) != 0;
        case bson_long: return bson_iterator_long_raw(i) != 0;
        case bson_double: return bson_iterator_double_raw(i) != 0;
        case bson_null: return 0;
        default: return 1;
    }
}

const char * bson_iterator_string( bson_iterator * i ){
    return bson_iterator_value( i ) + 4;
}
int bson_iterator_string_len( bson_iterator * i ){
    return bson_iterator_int_raw( i );
}

/* ----------------------------
   BUILDING
   ------------------------------ */

bson_buffer * bson_buffer_init( bson_buffer * b ){
    b->buf = (char*)malloc( initialBufferSize );
    if ( ! b->buf )
        return 0;
    b->bufSize = initialBufferSize;
    b->cur = b->buf + 4;
    b->finished = 0;
    b->stackPos = 0;
    return b;
}

void bson_append_byte( bson_buffer * b , char c ){
    b->cur[0] = c;
    b->cur++;
}
void bson_append( bson_buffer * b , const void * data , int len ){
    memcpy( b->cur , data , len );
    b->cur += len;
}
void bson_append32(bson_buffer * b, const void * data){
    bson_little_endian32(b->cur, data);
    b->cur += 4;
}
void bson_append64(bson_buffer * b, const void * data){
    bson_little_endian64(b->cur, data);
    b->cur += 8;
}

bson_buffer * bson_ensure_space( bson_buffer * b , const int bytesNeeded ){
    if ( b->finished )
        return 0;
    if ( b->bufSize - ( b->cur - b->buf ) > bytesNeeded )
        return b;
    b->buf = (char*)realloc( b->buf , (int)(1.5 * ( b->bufSize + bytesNeeded ) ) );
    if ( ! b->buf )
        return 0;
    return b;
}

char * bson_buffer_finish( bson_buffer * b ){
    int i;
    if ( ! b->finished ){
        if ( ! bson_ensure_space( b , 1 ) ) return 0;
        bson_append_byte( b , 0 );
        i = b->cur - b->buf;
        bson_little_endian32(b->buf, &i);
        b->finished = 1;
    }
    return b->buf;
}

void bson_buffer_destroy( bson_buffer * b ){
    free( b->buf );
    b->buf = 0;
    b->cur = 0;
    b->finished = 1;
}

bson_buffer * bson_append_estart( bson_buffer * b , int type , const char * name , const int dataSize ){
    if ( ! bson_ensure_space( b , 1 + strlen( name ) + 1 + dataSize ) )
        return 0;
    bson_append_byte( b , (char)type );
    bson_append( b , name , strlen( name ) + 1 );
    return b;
}

/* ----------------------------
   BUILDING TYPES
   ------------------------------ */

bson_buffer * bson_append_int( bson_buffer * b , const char * name , const int i ){
    if ( ! bson_append_estart( b , bson_int , name , 4 ) ) return 0;
    bson_append32( b , &i );
    return b;
}
bson_buffer * bson_append_double( bson_buffer * b , const char * name , const double d ){
    if ( ! bson_append_estart( b , bson_double , name , 8 ) ) return 0;
    bson_append64( b , &d );
    return b;
}
bson_buffer * bson_append_bool( bson_buffer * b , const char * name , const bson_bool_t i ){
    if ( ! bson_append_estart( b , bson_bool , name , 1 ) ) return 0;
    bson_append_byte( b , i != 0 );
    return b;
}
bson_buffer * bson_append_null( bson_buffer * b , const char * name ){
    if ( ! bson_append_estart( b , bson_null , name , 0 ) ) return 0;
    return b;
}
bson_buffer * bson_append_string( bson_buffer * b , const char * name , const char * value ){
    int sl = strlen( value ) + 1;
    if ( ! bson_append_estart( b , bson_string , name , 4 + sl ) ) return 0;
    bson_append32( b , &sl);
    bson_append( b , value , sl );
    return b;
}
bson_buffer * bson_append_oid( bson_buffer * b , const char * name , const bson_oid_t * oid ){
    if ( ! bson_append_estart( b , bson_oid , name , 12 ) ) return 0;
    bson_append( b , oid , 12 );
    return b;
}
bson_buffer * bson_append_new_oid( bson_buffer * b , const char * name ){
    bson_oid_t oid;
    bson_oid_gen(&oid);
    return bson_append_oid(b, name, &oid);
}


bson_buffer * bson_append_start_object( bson_buffer * b , const char * name ){
    int x = 0;
    if ( ! bson_append_estart( b , bson_object , name , 5 ) ) return 0;
    b->stack[ b->stackPos++ ] = b->cur;
    bson_append32( b , &x );
    return b;
}

bson_buffer * bson_append_start_array( bson_buffer * b , const char * name ){
    int x = 0;
    if ( ! bson_append_estart( b , bson_array , name , 5 ) ) return 0;
    b->stack[ b->stackPos++ ] = b->cur;
    bson_append32( b , &x );
    return b;
}

bson_buffer * bson_append_finish_object( bson_buffer * b ){
    char * start;
    int i;
    if ( ! bson_ensure_space( b , 1 ) ) return 0;
    bson_append_byte( b , 0 );
    
    start = b->stack[ --b->stackPos ];
    i = b->cur - start;
    bson_little_endian32(start, &i);

    return b;
}


void bson_fatal( char * msg , int ok ){
    if ( ok )
        return;
    fprintf( stderr , "bson error: %s\n" , msg );
    exit(-5);
}
