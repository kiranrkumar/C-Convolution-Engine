/*

Testing the basics of making a Max/MSP patch in C

https://cycling74.com/sdk/MaxSDK-6.0.4/html/index.html

*/

#include "ext.h"  // should always be first
#include "ext_obex.h"
#include <portaudio.h>
#include <sndfile.h>

/* Basic Max objects are declared as C structures. 
   The first element of the structure is a t_object, 
   followed by whatever you want*/
typedef struct _conv
{
	t_object s_obj;     // t_object header
	int s_value;        // something else
	char *audioFilename // another something else
	char *leftHRTFfilename // and another something else
	char *rightHRTFfilename // yet another something else
} t_conv;

/****** Initialization Routine *********/
/* 
	The initialization routine, which must be called main, 
	is called when Max loads your object for the first time. 
	In the initialization routine, you define one or more classes. 
	Defining a class consists of the following:

		1) telling Max about the size of your object's structure 
			and how to create and destroy an instance 
		2) defining methods that implement the object's behavior 
		3) in some cases, defining attributes that describe the object's data 
		4) registering the class in a name space 

	
	Example:
*/

static t_class *s_conv_class; // global pointer to our class definition that is setup in main()

int main()
{
	t_class *c;

	c = class_new("conv", (method)conv_new, (method)NULL, sizeof(t_conv), 0L, 0);   
	class_addmethod(c, (method)conv_int, "int", A_LONG, 0);
	class_addmethod(c, (method)conv_bang, "bang", 0);

	class_register(CLASS_BOX, c);

	s_conv_class = c;

	return 0;
}

/*************************************************
 ******************** conv_new *******************
 *************************************************
 * Allocate memory for, and create, a new instance
 * of the t_conv class
 *************************************************
 * PARAMETERS:	<none>
 *************************************************
 * RETURNS: 
 * 			x	-	new instance of the object
 *************************************************/
void *conv_new()
{
	t_conv *x = (t_conv *)object_alloc(s_conv_class);

	x->s_value = 0;

	return x;
}





// Define some methods that actually do something

/****************************************************************************************
 **************************************** conv_int **************************************
 ****************************************************************************************
 * Take an input int and store the value into the s_value member variable of the struct
 ****************************************************************************************
 * PARAMETERS:
 *			*x (t_conv) - pointer to the object. Required as the first parameter for all
 *						  functions
 *			n (long) - integer value from Max	
 *****************************************************************************************
 * RETURNS: 	<nothing>
 *****************************************************************************************/
void conv_int(t_conv *x, long n)
{
	x->s_value = n;
}

/****************************************************************************************
 *************************************** conv_bang **************************************
 ****************************************************************************************
 * Output the value of s_value to the Max window
 ****************************************************************************************
 * PARAMETERS:
 *			*x (t_conv) - pointer to the object. Required as the first parameter for all
 *						  functions
 *****************************************************************************************
 * RETURNS: 	<nothing>
 *****************************************************************************************/
void conv_bang(t_conv *x)
    {
        post("value is %ld",x->s_value);
    }
