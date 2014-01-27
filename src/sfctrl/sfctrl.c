/****************************************************
 tzcounter - a learning thing...
 ****************************************************/

/* Obligatory Pd header*/

#include "m_pd.h"

#include <stdio.h>
extern int zulu;



/* The class pointer */

t_class *sfctrl_class;	// tz

/* The object structure */

typedef struct _sfctrl {
  	t_object  x_obj;
  	t_int i_count;
	t_float f_frequency;
} t_sfctrl;

/* Function prototypes */

int list_radios(void);
int set_radio_frequency(int freq);


/* handles incoming bang - tz*/


void sfctrl_bang(t_sfctrl *x)
{
	
	// long sum;							// local variable for this method
	int n;
	
	n = list_radios();

	// sum = (long) n;	// return number of devices or -1 for error
	
  	t_float f = (t_float) n;
  	// x->i_count++;
  	
	outlet_float(x->x_obj.ob_outlet, f);


}

static void sfctrl_float(t_sfctrl *x, t_float f)
{
	char thing[100];
	
	int r;              // status return
	// x->p_value0 = n;					// store left operand value in instance's data structure

    r = set_radio_frequency((int) f );
	
	// post("hello float");
	sprintf(thing, "frequency set to %d, status: %d", (int) f, r );
	post(thing);
	x->f_frequency = f;		// just for the heck of it
   
}


void *sfctrl_new(t_floatarg f)
{
  	t_sfctrl *x = (t_sfctrl *)pd_new(sfctrl_class);

  	x->i_count=f;
  	outlet_new(&x->x_obj, &s_float);

	post("sfctrl added to patch");

  	return (void *)x;
}

void sfctrl_setup(void) {
  sfctrl_class = class_new(gensym("sfctrl"),
        (t_newmethod)sfctrl_new,
        0, sizeof(t_sfctrl),
        CLASS_DEFAULT,
        A_DEFFLOAT, 0);

  	class_addbang(sfctrl_class, sfctrl_bang);
	class_addfloat(sfctrl_class, (t_method)sfctrl_float);
}
