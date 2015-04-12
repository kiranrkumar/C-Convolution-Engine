/*

Testing the basics of making a Max/MSP patch in C

https://cycling74.com/sdk/MaxSDK-6.0.4/html/index.html

*/

#include "ext.h"  // should always be first
#include "ext_obex.h" //also required for Max
#include "z_dsp.h" // required for MSP work

#include <portaudio.h>
#include <sndfile.h>

/* Basic Max objects are declared as C structures. 
   The first element of the structure is a t_object, 
   followed by whatever you want*/
typedef struct _conv
{
	//t_object s_obj;     t_object header for Max-only objects
	t_pxobject s_obj;	  // MSP-specific object header
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

/* 
	Specify the signal processing function your object defines along with its arguments. 
	Called when the MSP signal compiler is building a sequence of operations 
	(known as the DSP Chain) that will be performed on each set of audio samples. 
	The operation sequence consists of a pointers to functions (called perform routines) 
	followed by arguments to those functions.
*/
void mydspobject_dsp(t_mydspobject *x, t_signal **sp, short *count);

int main()
{
	t_class *c;

	//create a new instance of the class
	c = class_new("conv", (method)conv_new, (method)dsp_free, sizeof(t_conv), 0L, 0); 
	//dsp_free (or some free function) is required for MSP
	
	/* add some standard method handlers for internal messages used by 
	all signal (MSP) objects */
	class_dspinit(c);
	
	//add method that is bound to the 'dsp' symbol
	class_addmethod(c, (method)mydspobject_dsp, "dsp", A_CANT, 0);
	
	//integer message sent to the object
	class_addmethod(c, (method)conv_int, "int", A_LONG, 0); 

	//bang message sent to the object
	class_addmethod(c, (method)conv_bang, "bang", 0); 

	//float message sent to the object
	class_addmethod(c, (method)conv_float, "float", A_FLOAT, 0); 

	//makes class searchable in Max by adding it to the CLASS_BOX namespace
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
	
	/* MSP-specific. Pass object pointer and number of inlets. Convolution would ideally have
	   3 inlets: audio source, leftHRTF, and rightHRTF
	*/
	dsp_setup((t_pxobject *)x, 3);
	
	// Create 2 outlets. We don't access them directly, so no need to store pointers
	outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");

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
	post("value is %ld",x->s_value); //post prints text in the Max window
}
    
/****************************************************************************************
 **************************************** conv_int **************************************
 ****************************************************************************************
 * Take an input float and prints it to the Max window
 ****************************************************************************************
 * PARAMETERS:
 *			*x (t_conv) - pointer to the object. Required as the first parameter for all
 *						  functions
 *			f (double) - float value from Max	
 *****************************************************************************************
 * RETURNS: 	<nothing>
 *****************************************************************************************/
void conv_float(t_conv *x, double f)
{
	post("got a float and it is %.2f", f);
}

/****************************************************************************************
 ******************************** mydspobject_dsp ***************************************
 ****************************************************************************************
 * 	Specify the signal processing function your object defines along with its arguments. 
 *	Called when the MSP signal compiler is building a sequence of operations 
 *	(known as the DSP Chain) that will be performed on each set of audio samples. 
 *	The operation sequence consists of a pointers to functions (called perform routines) 
 *	followed by arguments to those functions.
 ****************************************************************************************
 * PARAMETERS:
 *			*x (t_mydspobject) - pointer to the signal processing object
 *						  functions
 *			**sp (t_signal) - pointer to an array of t_signal object pointers. Two important
 *							  elements in t_signal are:
 *							  - s_n = size of the signal vector
 *							  - s_vec = the actual signal data in a 32-bit float array
 *
 *							  sp has the inputs and outputs. E.g., for 2 inputs and 3 outputs,
 *							  it would look like this:
									sp[0] // left input
									sp[1] // right input
									sp[2] // left output
									sp[3] // middle output
									sp[4] // right output 
 *			*count (short) - 
 *****************************************************************************************
 * RETURNS: 	<nothing>
 *****************************************************************************************/
void mydspobject_dsp(t_mydspobject *x, t_signal **sp, short *count)
{

}


/*
Use dsp_add() to add an entry to the signal chain. Can use multiple strategies. For simple
unit generators not storing internal state between computing vectors, you can just pass the 
inputs, outputs, and vector size. For objects that need to store internal state between 
computing vectors (e.g. filters or ramp generators), you will pass a pointer to your object, 
whose data structure should contain space to store this state.

The plus1~ object below does not need to store internal state. It passes the input, output, 
and vector size to its perform routine. The plus1~ dsp method is shown below:
*/
void plus1_dsp(t_plus1 *x, t_signal **sp, short *count)
{
	/* dsp_add arguments
			- perform routine
			- number of arguments
			- the arguments themselves
	*/
	dsp_add(plus1_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

/*The perform routine is not a "method" in the traditional sense. It will be called 
within the callback of an audio driver, which, unless the user is employing the Non-Real 
Time audio driver, will typically be in a high-priority thread. Thread protection inside 
the perform routine is minimal. You can use a clock, but you cannot use qelems or outlets.
The design of the perform routine is somewhat unlike other Max methods. It receives a 
pointer to a piece of the DSP chain and it is expected to return the location of the next 
perform routine on the chain. The next location is determined by the number of arguments 
you specified for your perform routine with your call to dsp_add(). For example, if you 
will pass three arguments, you need to return w + 4.*/
t_int *plus1_perform(t_int *w)
{
	t_float *in, *out;
	int n;

	in = (t_float *)w[1];       // get input signal vector
	out = (t_float *)w[2];      // get output signal vector
	n = (int)w[3];          // vector size


	while (n--)         // perform calculation on all samples
		*out++ = *in++ + 1.;

	return w + 4;           // must return next DSP chain location
}

/* The free function for the class must either be dsp_free() or it must be written to 
call dsp_free() as below: */
void mydspobject_free(t_mydspobject *x)
{
	dsp_free((t_pxobject *)x);

	// can do other stuff here
}