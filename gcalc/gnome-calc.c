/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* GnomeCalc - double precision simple calculator widget
 *
 * Author: George Lebl <jirka@5z.com>
 */

#include <config.h>

/* needed for values of M_E and M_PI under 'gcc -ansi -pedantic'
 * on GNU/Linux */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <errno.h> /* errno */
#include <signal.h> /* signal() */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-macros.h>

#include "gnome-calc.h"
#include "sr.h"

#undef GNOME_CALC_DEBUG

typedef void (*sighandler_t)(int);

struct _GnomeCalcPrivate {
	GtkWidget *text_display;
	GtkWidget *store_display;

	GtkWidget *invert_button;
	GtkWidget *drg_button;

	GList *stack;
	GtkAccelGroup *accel;

	gdouble result;
	gdouble memory;

	gchar result_string[13];

	GnomeCalcMode mode : 2;

	guint add_digit : 1;	/*add a digit instead of starting a new
				  number*/
	guint error : 1;
	guint invert : 1;
	
	GtkTooltips *tooltips;
};

typedef enum {
	CALCULATOR_NUMBER,
	CALCULATOR_FUNCTION,
	CALCULATOR_PARENTHESIS
} CalculatorActionType;

typedef gdouble (*MathFunction1) (gdouble);
typedef gdouble (*MathFunction2) (gdouble, gdouble);

typedef struct _CalculatorStack CalculatorStack;
struct _CalculatorStack {
	CalculatorActionType type;
	union {
		MathFunction2 func;
		gdouble number;
	} d;
};




static void gnome_calc_class_init	(GnomeCalcClass	*class);
static void gnome_calc_instance_init	     (GnomeCalc	*gc);
static void gnome_calc_destroy	(GtkObject    	*object);
static void gnome_calc_finalize	(GObject		*object);

typedef struct _CalculatorButton CalculatorButton;
struct _CalculatorButton {
	char *name;
	GCallback signal_func;
	gpointer data;
	gpointer invdata;
	gboolean convert_to_rad;
	guint keys[10]; /*key shortcuts, 0 terminated list,
			  make sure to increase the number if more is needed*/
	char *tooltip;
	char *tooltip_private;
};

typedef void (*GnomeCalcualtorResultChangedSignal) (GtkObject * object,
						    gdouble result,
						    gpointer data);


enum {
	RESULT_CHANGED_SIGNAL,
	LAST_SIGNAL
};

static gint gnome_calc_signals[LAST_SIGNAL];

static gboolean cmd_saved = FALSE;

GNOME_CLASS_BOILERPLATE (GnomeCalc, gnome_calc,
			 GtkVBox, GTK_TYPE_VBOX)

static void
gnome_calc_class_init (GnomeCalcClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	object_class->destroy = gnome_calc_destroy;
	gobject_class->finalize = gnome_calc_finalize;

	gnome_calc_signals[RESULT_CHANGED_SIGNAL] =
		gtk_signal_new("result_changed",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE(object_class),
			       GTK_SIGNAL_OFFSET(GnomeCalcClass,
			       			 result_changed),
			       g_cclosure_marshal_VOID__DOUBLE,
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_DOUBLE);

	class->result_changed = NULL;
}

#ifdef GNOME_CALC_DEBUG 
static void
dump_stack(GnomeCalc *gc)
{
	CalculatorStack *stack;
	GList *list;

	g_print ("STACK_DUMP start, we are in ");
	if (gc->_priv->add_digit)
		g_print ("add digit mode.\n");
	else
		g_print ("normal mode\n");
		
	for(list = gc->_priv->stack;list!=NULL;list = g_list_next(list)) {
		stack = list->data;
		if(stack == NULL)
			puts("NULL");
		else if(stack->type == CALCULATOR_PARENTHESIS)
			puts("(");
		else if(stack->type == CALCULATOR_NUMBER)
			printf("% .12g\n", stack->d.number);
		else if(stack->type == CALCULATOR_FUNCTION)
			puts("FUNCTION");
		else
			puts("UNKNOWN");
	}
	puts("STACK_DUMP end\n");
}
#endif

static void
stack_pop(GList **stack)
{
	CalculatorStack *s;
	GList *p;

	g_return_if_fail(stack);

	if(*stack == NULL) {
		g_warning("Stack underflow!");
		return;
	}

	s = (*stack)->data;
	p = (*stack)->next;
	g_list_free_1(*stack);
	if(p) p->prev = NULL;
	*stack = p;
	g_free(s);
}

static void
set_display (GnomeCalc *gc)
{
        GtkWidget *string_text = gc->_priv->text_display;
        GtkWidget *store_text = gc->_priv->store_display;
        GtkTextBuffer *buffer;
        GtkTextIter start, end;

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (string_text));
        gtk_text_buffer_get_bounds (buffer, &start, &end);
        gtk_text_buffer_delete (buffer, &start, &end);
        gtk_text_buffer_insert_with_tags_by_name (buffer,
                                                  &end,
                                                  gc->_priv->result_string,
                                                  -1,
                                                  "x-large",
                                                  NULL);
        
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (store_text));
        gtk_text_buffer_get_bounds (buffer, &start, &end);
        gtk_text_buffer_delete (buffer, &start, &end);
        gtk_text_buffer_insert_with_tags_by_name (buffer,
                                                  &end,
                                                  (gc->_priv->memory != 0.0) ? "m" : " ",
                                                  -1,
                                                  "x-large",
                                                  NULL);

}

static void
do_error(GnomeCalc *gc)
{
	gc->_priv->error = TRUE;
	if (errno == ERANGE)
		strcpy(gc->_priv->result_string,_("inf"));
	else
		strcpy(gc->_priv->result_string,"e");

	set_display (gc);


}

/*we handle sigfpe's so that we can find all the errors*/
static void
sigfpe_handler(int type)
{
	/*most likely, but we don't really care what the value is*/
	errno = ERANGE;
}

static void
reduce_stack(GnomeCalc *gc)
{
	CalculatorStack *stack;
	GList *list;
	MathFunction2 func;
	gdouble first;
	gdouble second;

	if(!gc->_priv->stack)
		return;

	stack = gc->_priv->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return;

	second = stack->d.number;

	list=g_list_next(gc->_priv->stack);
	if(!list)
		return;

	stack = list->data;
	if(stack->type==CALCULATOR_PARENTHESIS)
		return;
	if(stack->type!=CALCULATOR_FUNCTION) {
		g_warning("Corrupt GnomeCalc stack!");
		return;
	}
	func = stack->d.func;

	list=g_list_next(list);
	if(!list) {
		g_warning("Corrupt GnomeCalc stack!");
		return;
	}

	stack = list->data;
	if(stack->type!=CALCULATOR_NUMBER) {
		g_warning("Corrupt GnomeCalc stack!");
		return;
	}
	first = stack->d.number;

	stack_pop(&gc->_priv->stack);
	stack_pop(&gc->_priv->stack);

	errno = 0;

	{
		sighandler_t old = signal(SIGFPE,sigfpe_handler);
		stack->d.number = (*func)(first,second);
		signal(SIGFPE,old);
	}

	if(errno>0 ||
	   finite(stack->d.number)==0) {
	   	do_error(gc);
		stack->d.number = 0;
		errno = 0;		
	}
}

/* Move these up for find_precedence(). */
static gdouble
c_add(gdouble arg1, gdouble arg2)
{
	return arg1+arg2;
}

static gdouble
c_sub(gdouble arg1, gdouble arg2)
{
	return arg1-arg2;
}

static gdouble
c_mul(gdouble arg1, gdouble arg2)
{
	return arg1*arg2;
}

static gdouble
c_div(gdouble arg1, gdouble arg2)
{
	if(arg2==0) {
		errno=ERANGE;
		return 0;
	}
	return arg1/arg2;
}

static int
find_precedence(MathFunction2 f)
{
        if ( f == NULL || f == c_add || f == c_sub)
                return 0;
        else if ( f == c_mul || f == c_div )
                return 1;
        else
                return 2;
}

static void
reduce_stack_prec(GnomeCalc *gc, MathFunction2 func)
{
        CalculatorStack *stack;
        GList *list;

        stack = gc->_priv->stack->data;
        if ( stack->type != CALCULATOR_NUMBER )
                return;

        list = g_list_next(gc->_priv->stack);
        if (!list)
                return;

        stack = list->data;
        if ( stack->type == CALCULATOR_PARENTHESIS )
                return;

        if ( find_precedence(func) <= find_precedence(stack->d.func) ) {
                reduce_stack(gc);
                reduce_stack_prec(gc,func);
        }

        return;
}

static void
push_input(GnomeCalc *gc)
{
	if(gc->_priv->add_digit) {
		CalculatorStack *stack;
		stack = g_new(CalculatorStack,1);
		stack->type = CALCULATOR_NUMBER;
		stack->d.number = gc->_priv->result;
		gc->_priv->stack = g_list_prepend(gc->_priv->stack,stack);
		gc->_priv->add_digit = FALSE;
	}
}

static void
set_result(GnomeCalc *gc)
{
	CalculatorStack *stack;
	gchar buf[80];
	gchar format[20];
	gint i;

	g_return_if_fail(gc!=NULL);

	if(!gc->_priv->stack)
		return;

	stack = gc->_priv->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return;

	gc->_priv->result = stack->d.number;
	
        /* make sure put values in a consistent manner */
	/* XXX: perhaps we can make sure the calculator works on all locales,
	 * but currently it will lose precision if we don't do this */
	gnome_i18n_push_c_numeric_locale ();
	for (i = 12; i > 0; i--) {
		g_snprintf (format, sizeof (format), "%c .%dg", '%', i);
		g_snprintf (buf, sizeof (buf), format, gc->_priv->result);
		if (strlen (buf) <= 12)
			break;
	}
	gnome_i18n_pop_c_numeric_locale ();
	strncpy(gc->_priv->result_string,buf,12);
	gc->_priv->result_string[12]='\0';

	set_display (gc);
	gtk_signal_emit(GTK_OBJECT(gc),
			gnome_calc_signals[RESULT_CHANGED_SIGNAL],
			gc->_priv->result);

}

static void
unselect_invert(GnomeCalc *gc)
{
	g_return_if_fail(gc != NULL);
	g_return_if_fail(GNOME_IS_CALC(gc));
	g_return_if_fail(gc->_priv->invert_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gc->_priv->invert_button),
				    FALSE);
	gc->_priv->invert=FALSE;
}

static void
setup_drg_label(GnomeCalc *gc)
{
	GtkButton *button;

	g_return_if_fail(gc != NULL);
	g_return_if_fail(GNOME_IS_CALC(gc));
	g_return_if_fail(gc->_priv->drg_button);

	button = GTK_BUTTON(gc->_priv->drg_button);

	if(gc->_priv->mode == GNOME_CALC_DEG)
		gtk_button_set_label(button, _("DEG"));
	else if(gc->_priv->mode == GNOME_CALC_RAD)
		gtk_button_set_label(button, _("RAD"));
	else
		gtk_button_set_label(button, _("GRAD"));
}

static gdouble
convert_num(gdouble num, GnomeCalcMode from, GnomeCalcMode to)
{
	if(to==from)
		return num;
	else if(from==GNOME_CALC_DEG) {
		if(to==GNOME_CALC_RAD) {
			if (fabs (num - 90) < 0.000001)
				return M_PI_2;
			else if (fabs (num - 180) < 0.000001)
				return M_PI;
			else if (fabs (num - 45) < 0.000001)
				return M_PI_4;
			else if (fabs (num - 360) < 0.000001)
				return 2*M_PI;
			return (num*M_PI)/180;
		} else /*GRAD*/
			return (num*200)/180;
	}
	else if(from==GNOME_CALC_RAD)
		if(to==GNOME_CALC_DEG) {
			if (num == M_PI)
				return 180;
			else if (num == M_PI_2)
				return 90;
			else if (num == M_PI_4)
				return 45;
			return (num*180)/M_PI;
		} else /*GRAD*/
			return (num*200)/M_PI;
	else /*GRAD*/
		if(to==GNOME_CALC_DEG)
			return (num*180)/200;
		else /*RAD*/
			return (num*M_PI)/200;
}

static void
no_func(GtkWidget *w, gpointer data)
{
	GList *list;
	static CalculatorStack saved_func, saved_num;
	CalculatorStack *stack1, *stack2, *add_func, *add_num;
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	push_input(gc);

	/* if no stack, nothing happens */
	if(!gc->_priv->stack) {
		unselect_invert(gc);
		return;
	}

	reduce_stack_prec(gc,NULL);

	if(gc->_priv->error) {
		gc->_priv->error = FALSE;
		return;
	}

	/* = after <math_func>=  eg: 3+== */
	if (cmd_saved == TRUE) {
		add_func = g_new(CalculatorStack, 1);
		add_func->type = saved_func.type;
		add_func->d = saved_func.d;
		gc->_priv->stack = g_list_prepend(gc->_priv->stack, add_func);

		add_num = g_new(CalculatorStack, 1);
		add_num->type = saved_num.type;
		add_num->d = saved_num.d;
		gc->_priv->stack = g_list_prepend(gc->_priv->stack, add_num);
		gc->_priv->add_digit = FALSE;

		reduce_stack(gc);
	} else {
		if(gc->_priv->stack == NULL)
			return;
		stack1 = gc->_priv->stack->data;

		list=g_list_next(gc->_priv->stack);
		if(list) {
			stack2 = list->data;

			if( stack1->type == CALCULATOR_FUNCTION && 
		    		stack2->type == CALCULATOR_NUMBER) {
				saved_func.type = CALCULATOR_FUNCTION;
				saved_func.d = stack1->d;

				saved_num.type = CALCULATOR_NUMBER;
				saved_num.d = stack2->d;

				add_num = g_new(CalculatorStack,1);
				add_num->type = saved_num.type;
				add_num->d = saved_num.d;
				gc->_priv->stack = g_list_prepend(gc->_priv->stack, add_num);
				gc->_priv->add_digit = FALSE;

				reduce_stack(gc);
				cmd_saved = TRUE;
			}
		}
	}

	set_result(gc);

	unselect_invert(gc);
}

static void
simple_func(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	CalculatorStack *stack;
	CalculatorButton *but = data;
	MathFunction1 func = but->data;
	MathFunction1 invfunc = but->invdata;

	g_return_if_fail(func!=NULL);
	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	push_input(gc);

	if(!gc->_priv->stack) {
		unselect_invert(gc);
		return;
	}

	stack = gc->_priv->stack->data;

	if(stack->type==CALCULATOR_FUNCTION) {
		stack_pop(&gc->_priv->stack);
		stack = gc->_priv->stack->data;
	}

	if(stack->type!=CALCULATOR_NUMBER) {
		unselect_invert(gc);
		return;
	}
	
	/*only convert non inverting functions*/
	if(!gc->_priv->invert && but->convert_to_rad)
		stack->d.number = convert_num(stack->d.number,
					      gc->_priv->mode,
					      GNOME_CALC_RAD);
	
	errno = 0;
	{
		sighandler_t old = signal(SIGFPE,sigfpe_handler);
		if(!gc->_priv->invert || invfunc==NULL)
			stack->d.number = (*func)(stack->d.number);
		else
			stack->d.number = (*invfunc)(stack->d.number);
		signal(SIGFPE,old);
	}
	
	if(errno>0 ||
	   finite(stack->d.number)==0) {
	   	do_error(gc);
		gc->_priv->error = FALSE;
		stack->d.number = 0;
		errno = 0;
		return;
	}
	
	/*we are converting back from rad to mode*/
	if(gc->_priv->invert && but->convert_to_rad)
		stack->d.number = convert_num(stack->d.number,
					      GNOME_CALC_RAD,
					      gc->_priv->mode);
	
	set_result(gc);

	unselect_invert(gc);
}

static void
math_func(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	CalculatorStack *stack;
	CalculatorButton *but = data;
	MathFunction2 func = but->data;
	MathFunction2 invfunc = but->invdata;

	g_return_if_fail(func!=NULL);
	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	push_input(gc);

	if(!gc->_priv->stack) {
		unselect_invert(gc);
		return;
	}

	reduce_stack_prec(gc,func);
	if(gc->_priv->error) return;
	set_result(gc);

	stack = gc->_priv->stack->data;

	if(stack->type==CALCULATOR_FUNCTION) {
		stack_pop(&gc->_priv->stack);
		stack = gc->_priv->stack->data;
	}

	if(stack->type!=CALCULATOR_NUMBER) {
		unselect_invert(gc);
		return;
	}

	stack = g_new(CalculatorStack,1);
	stack->type = CALCULATOR_FUNCTION;
	if(!gc->_priv->invert || invfunc==NULL)
		stack->d.func = func;
	else
		stack->d.func = invfunc;

	gc->_priv->stack = g_list_prepend(gc->_priv->stack,stack);

	unselect_invert(gc);
}

static void
reset_calc(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc;
	if(w)
		gc = g_object_get_data(G_OBJECT(w), "set_data");
	else
		gc = data;

	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	while(gc->_priv->stack)
		stack_pop(&gc->_priv->stack);

	gc->_priv->result = 0;
	strcpy(gc->_priv->result_string, " 0");
	gc->_priv->memory = 0;
	gc->_priv->mode = GNOME_CALC_DEG;
	gc->_priv->invert = FALSE;
	gc->_priv->error = FALSE;

	gc->_priv->add_digit = TRUE;
	push_input(gc);
	set_result(gc);
	unselect_invert(gc);
	setup_drg_label(gc);
}

static void
clear_calc(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	/* if in add digit mode, just clear the number, otherwise clear
	 * state as well */
	if(!gc->_priv->add_digit) {
		while(gc->_priv->stack)
			stack_pop(&gc->_priv->stack);
	}

	gc->_priv->result = 0;
	strcpy(gc->_priv->result_string, " 0");
	gc->_priv->error = FALSE;
	gc->_priv->invert = FALSE;

	gc->_priv->add_digit = TRUE;
	push_input(gc);
	set_result(gc);

	unselect_invert(gc);
}


/**
 * gnome_calc_clear
 * @gc: Pointer to GNOME calculator widget.
 * @reset: %FALSE to zero, %TRUE to reset calculator completely
 *
 * Description:
 * Resets the calculator back to zero.  If @reset is %TRUE, results
 * stored in memory and the calculator mode are cleared also.
 **/

void
gnome_calc_clear(GnomeCalc *gc, const gboolean reset)
{
	if (reset)
		reset_calc(NULL, gc);
	else
		clear_calc(NULL, gc);
}

static void
add_digit (GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data (G_OBJECT (w), "set_data");
	CalculatorButton *but = data;
	gchar *digit = but->name;

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	/* For "EE" where name is not what we want to add */
	if (but->data != NULL)
		digit = but->data;

	g_return_if_fail(gc!=NULL);
	g_return_if_fail(digit!=NULL);

	if(!gc->_priv->add_digit) {
		if(gc->_priv->stack) {
			CalculatorStack *stack=gc->_priv->stack->data;
			if(stack->type==CALCULATOR_NUMBER)
				stack_pop(&gc->_priv->stack);
		}
		gc->_priv->add_digit = TRUE;
		gc->_priv->result_string[0] = '\0';
	}

	unselect_invert(gc);

	if(digit[0]=='e') {
		if(strchr(gc->_priv->result_string,'e'))
			return;
		else if(strlen(gc->_priv->result_string)>9)
			return;
		else if(gc->_priv->result_string[0]=='\0')
			strcpy(gc->_priv->result_string," 0");
	} else if(digit[0]=='.') {
		if(strchr(gc->_priv->result_string,'.'))
			return;
		else if(strlen(gc->_priv->result_string)>10)
			return;
		else if(gc->_priv->result_string[0]=='\0')
			strcpy(gc->_priv->result_string," 0");
	} else { /*numeric*/
		if(strlen(gc->_priv->result_string)>11)
			return;
		else if (strcmp (gc->_priv->result_string, " 0") == 0 ||
			 gc->_priv->result_string[0]=='\0')
			strcpy(gc->_priv->result_string," ");
	}

	strcat (gc->_priv->result_string, digit);

	set_display (gc);
 	
        /* make sure get values in a consistent manner */
	gnome_i18n_push_c_numeric_locale ();
	sscanf(gc->_priv->result_string, "%lf", &gc->_priv->result);
	gnome_i18n_pop_c_numeric_locale ();
}

static gdouble
c_neg(gdouble arg)
{
	return -arg;
}


static void
negate_val(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	char *p;

	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	unselect_invert(gc);

	if(!gc->_priv->add_digit && gc->_priv->result != 0) {
		simple_func(w,data);
		return;
	}

	if((p=strchr(gc->_priv->result_string,'e'))!=NULL) {
		p++;
		if(*p=='-')
			*p='+';
		else
			*p='-';
	} else {
		if(gc->_priv->result_string[0]=='-' || 
		   gc->_priv->result == 0) 
			gc->_priv->result_string[0]=' ';
		else 
			gc->_priv->result_string[0]='-';
	}

        /* make sure get values in a consistent manner */
	gnome_i18n_pop_c_numeric_locale ();
	sscanf(gc->_priv->result_string, "%lf", &gc->_priv->result);
	gnome_i18n_push_c_numeric_locale ();

	set_display (gc);
}

static gdouble
c_inv(gdouble arg1)
{
	if(arg1==0) {
		errno=ERANGE;
		return 0;
	}
	return 1/arg1;
}

static gdouble
c_pow2(gdouble arg1)
{
	return pow(arg1,2);
}

static gdouble
c_pow10(gdouble arg1)
{
	return pow(10,arg1);
}

static gdouble
c_powe(gdouble arg1)
{
	return pow(M_E,arg1);
}

static gdouble
c_fact(gdouble arg1)
{
	int i;
	gdouble r;
	if(arg1<0) {
		errno=ERANGE;
		return 0;
	}
	i = (int)(arg1+0.5);
	if((fabs(arg1-i))>1e-9) {
		errno=ERANGE;
		return 0;
	}
	for(r=1;i>0;i--)
		r*=i;

	return r;
}

static gdouble
set_result_to(GnomeCalc *gc, gdouble result)
{
	gdouble old;

	if(gc->_priv->stack==NULL ||
	   ((CalculatorStack *)gc->_priv->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->_priv->add_digit = TRUE;
		old = gc->_priv->result;
		gc->_priv->result = result;
		push_input(gc);
	} else {
		old = ((CalculatorStack *)gc->_priv->stack->data)->d.number;
		((CalculatorStack *)gc->_priv->stack->data)->d.number = result;
	}

	set_result(gc);

	return old;
}


/**
 * gnome_calc_set
 * @gc: Pointer to GNOME calculator widget.
 * @result: New value of calculator buffer.
 *
 * Description:  Sets the value stored in the calculator's result buffer to the
 * given @result.
 **/

void
gnome_calc_set(GnomeCalc *gc, gdouble result)
{
	set_result_to(gc,result);
}

static void
store_m(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	push_input(gc);

	gc->_priv->memory = gc->_priv->result;

	set_display (gc);
 	
	unselect_invert(gc);
}

static void
recall_m(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	set_result_to(gc,gc->_priv->memory);

	unselect_invert(gc);
}

static void
sum_m(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	
	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	push_input(gc);

	gc->_priv->memory += gc->_priv->result;

	set_display (gc);
 	
	unselect_invert(gc);
}

static void
exchange_m(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	gc->_priv->memory = set_result_to(gc,gc->_priv->memory);

	unselect_invert(gc);
}

static void
invert_toggle(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	if(GTK_TOGGLE_BUTTON(w)->active)
		gc->_priv->invert=TRUE;
	else
		gc->_priv->invert=FALSE;
}

static void
drg_toggle(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	GnomeCalcMode oldmode;

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	oldmode = gc->_priv->mode;

	if(gc->_priv->mode==GNOME_CALC_DEG)
		gc->_priv->mode=GNOME_CALC_RAD;
	else if(gc->_priv->mode==GNOME_CALC_RAD)
		gc->_priv->mode=GNOME_CALC_GRAD;
	else
		gc->_priv->mode=GNOME_CALC_DEG;

	setup_drg_label(gc);

	/*convert if invert is on*/
	if(gc->_priv->invert) {
		CalculatorStack *stack;
		push_input(gc);
		stack = gc->_priv->stack->data;
		stack->d.number = convert_num(stack->d.number,
					      oldmode,gc->_priv->mode);
		set_result(gc);
	}

	unselect_invert(gc);
}

static void
set_pi(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	if(gc->_priv->error)
		return;

	set_result_to(gc,M_PI);

	unselect_invert(gc);
}

static gboolean
maybe_run_slide_rule (GtkWidget *w, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_3BUTTON_PRESS) {
		run_slide_rule ();
		return TRUE;
	}
	return FALSE;
}

static void
set_e (GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);
	
	if(gc->_priv->error)
		return;

	set_result_to(gc,M_E);

	unselect_invert(gc);
}

static void
add_parenth(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");

	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	if(gc->_priv->stack &&
	   ((CalculatorStack *)gc->_priv->stack->data)->type==CALCULATOR_NUMBER)
		((CalculatorStack *)gc->_priv->stack->data)->type =
			CALCULATOR_PARENTHESIS;
	else {
		CalculatorStack *stack;
		stack = g_new(CalculatorStack,1);
		stack->type = CALCULATOR_PARENTHESIS;
		gc->_priv->stack = g_list_prepend(gc->_priv->stack,stack);
	}

	unselect_invert(gc);
}

static void
sub_parenth(GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	g_return_if_fail(gc!=NULL);

	cmd_saved = FALSE;

	if(gc->_priv->error)
		return;

	push_input(gc);
	reduce_stack_prec(gc,NULL);
	if(gc->_priv->error) return;
	set_result(gc);

	if(gc->_priv->stack) {
		CalculatorStack *stack = gc->_priv->stack->data;
		if(stack->type==CALCULATOR_PARENTHESIS)
			stack_pop(&gc->_priv->stack);
		else if(g_list_next(gc->_priv->stack)) {
			stack = g_list_next(gc->_priv->stack)->data;
			if(stack->type==CALCULATOR_PARENTHESIS) {
				GList *n = g_list_next(gc->_priv->stack);
				gc->_priv->stack = g_list_remove_link(gc->_priv->stack,n);
				g_list_free_1(n);
			}
		}
	}

	unselect_invert(gc);
}

static double
sin_helper (double value)
{
	double ret = sin (value);
	
	if (fabs (ret) <= fabs (sin (M_PI))) 
		return 0.0;
	else
		return ret;
}

static double
cos_helper (double value)
{
	double ret = cos (value);
	if (fabs (ret) <= fabs (cos (M_PI_2)))
		return 0.0;
	else
		return ret;
}

static double
tan_helper (double value)
{
	double ret = tan (value);
	double temp1, temp2;

	if (fabs (ret) >= fabs (tan(3 * M_PI_2))) {
		errno=ERANGE;
		return 0.0;
	}
	else if (fabs (ret) <= fabs (tan(M_PI))) {
		return 0.0;
	}
	else
		return ret;
}

static const CalculatorButton buttons[8][5] = {
	{
		{N_("1/x"),  (GtkSignalFunc)simple_func, c_inv,  NULL,   FALSE, {0}, N_("Inverse"), NULL },
		{N_("x<sup>2</sup>"),  (GtkSignalFunc)simple_func, c_pow2, sqrt,   FALSE, {0}, N_("Square"), NULL },
		{N_("SQRT"), (GtkSignalFunc)simple_func, sqrt,   c_pow2, FALSE, {'r','R',0}, N_("Square root"), NULL },
		{N_("CE/C"), (GtkSignalFunc)clear_calc,  NULL,   NULL,   FALSE, {GDK_Clear,GDK_Delete,0}, N_("Clear"), NULL },
		{N_("AC"),   (GtkSignalFunc)reset_calc,  NULL,   NULL,   FALSE, {'a','A', GDK_Escape, 0}, N_("Reset"), NULL }
	},{
		{N_("INV"),  NULL,                       NULL,   NULL,   FALSE, {'i','I',0}, N_("Inverse trigonometry/log function"), NULL }, /*inverse button*/
		{N_("sin"),  (GtkSignalFunc)simple_func, sin_helper, asin, TRUE,{'s','S',0}, N_("Sine"), NULL },
		{N_("cos"),  (GtkSignalFunc)simple_func, cos_helper, acos, TRUE,{'c','C',0}, N_("Cosine"), NULL },
		{N_("tan"),  (GtkSignalFunc)simple_func, tan_helper,    atan,   TRUE,  {'t','T',0}, N_("Tangent"), NULL },
		/* Ugly hack so that window won't resize when selecting GRAD - there should be a better fix that this */
		{N_(" DEG  "),  (GtkSignalFunc)drg_toggle,  NULL,   NULL,   FALSE, {'d','D',0}, N_("Switch degrees / radians / grad"), NULL }
	},{
		{N_("e"),    (GtkSignalFunc)set_e,       NULL,   NULL,   FALSE, {'e','E',0}, N_("Base of Natural Logarithm"), NULL },
		{N_("EE"),   (GtkSignalFunc)add_digit,   "e+",   NULL,   FALSE, {0}, N_("Add digit"), NULL },
		{N_("log"),  (GtkSignalFunc)simple_func, log10,  c_pow10,FALSE, {0}, N_("Base 10 Logarithm"), NULL },
		{N_("ln"),   (GtkSignalFunc)simple_func, log,    c_powe, FALSE, {'l','L',0}, N_("Natural Logarithm"), NULL },
		{N_("x<sup>y</sup>"),  (GtkSignalFunc)math_func,   pow,    NULL,   FALSE, {'^',0}, N_("Power"), NULL }
	},{
		{N_("PI"),   (GtkSignalFunc)set_pi,      NULL,   NULL,   FALSE, {'p','P',0}, N_("PI"), NULL },
		{N_("x!"),   (GtkSignalFunc)simple_func, c_fact, NULL,   FALSE, {'!',0}, N_("Factorial"), NULL },
		{N_("("),    (GtkSignalFunc)add_parenth, NULL,   NULL,   FALSE, {'(',0}, N_("Opening Parenthesis"), NULL },
		{N_(")"),    (GtkSignalFunc)sub_parenth, NULL,   NULL,   FALSE, {')',0}, N_("Closing Parenthesis"), NULL },
		{N_("/"),    (GtkSignalFunc)math_func,   c_div,  NULL,   FALSE, {'/',GDK_KP_Divide,0}, N_("Divide by"), NULL }
	},{
		{N_("STO"),  (GtkSignalFunc)store_m,     NULL,   NULL,   FALSE, {0}, N_("Store the value in the\ndisplay field in memory"), NULL },
		{N_("7"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'7',GDK_KP_7,GDK_KP_Home,0}, NULL, NULL },
		{N_("8"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'8',GDK_KP_8,GDK_KP_Up,0}, NULL, NULL },
		{N_("9"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'9',GDK_KP_9,GDK_KP_Page_Up,0}, NULL, NULL },
		{N_("*"),    (GtkSignalFunc)math_func,   c_mul,  NULL,   FALSE, {'*',GDK_KP_Multiply,0}, N_("Multiply by"), NULL }
	},{
		{N_("RCL"),  (GtkSignalFunc)recall_m,    NULL,   NULL,   FALSE, {0}, N_("Display the value in memory\nin the display field"), NULL },
		{N_("4"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'4',GDK_KP_4,GDK_KP_Left,0}, NULL, NULL },
		{N_("5"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'5',GDK_KP_5,GDK_KP_Begin,0}, NULL, NULL },
		{N_("6"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'6',GDK_KP_6,GDK_KP_Right,0}, NULL, NULL },
		{N_("-"),    (GtkSignalFunc)math_func,   c_sub,  NULL,   FALSE, {'-',GDK_KP_Subtract,0}, N_("Subtract"), NULL }
	},{
		{N_("SUM"),  (GtkSignalFunc)sum_m,       NULL,   NULL,   FALSE, {0}, N_("Add the value in the display\nfield to the value in memory"), NULL },
		{N_("1"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'1',GDK_KP_1,GDK_KP_End,0}, NULL, NULL },
		{N_("2"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'2',GDK_KP_2,GDK_KP_Down,0}, NULL, NULL },
		{N_("3"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'3',GDK_KP_3,GDK_KP_Page_Down,0}, NULL, NULL },
		{N_("+"),    (GtkSignalFunc)math_func,   c_add,  NULL,   FALSE, {'+',GDK_KP_Add,0}, N_("Add"), NULL }
	},{
		{N_("EXC"),  (GtkSignalFunc)exchange_m,  NULL,   NULL,   FALSE, {0}, N_("Exchange the values in the\ndisplay field and memory"), NULL },
		{N_("0"),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'0',GDK_KP_0,GDK_KP_Insert,0}, NULL, NULL },
		{N_("."),    (GtkSignalFunc)add_digit,   NULL,   NULL,   FALSE, {'.',GDK_KP_Decimal,',',GDK_KP_Delete,0}, N_("Decimal Point"), NULL },
		{N_("+/-"),  (GtkSignalFunc)negate_val,  c_neg,  NULL,   FALSE, {0}, N_("Change sign"), NULL },
		{N_("="),    (GtkSignalFunc)no_func,     NULL,   NULL,   FALSE, {'=',GDK_KP_Enter,0}, N_("Calculate"), NULL }
	}
};

static void
create_button(GnomeCalc *gc, GtkWidget *table, int x, int y)
{
	const CalculatorButton *but = &buttons[y][x];
	GtkWidget *w, *l;
	int i;

	if(!but->name)
		return;

	l = gtk_label_new (_(but->name));
	gtk_widget_show (l);
	gtk_label_set_use_markup (GTK_LABEL (l), TRUE);

	if (strcmp (but->name, "INV") == 0) {
		w = gtk_toggle_button_new ();
		gtk_container_add (GTK_CONTAINER (w), l);
		gc->_priv->invert_button = w;
		g_signal_connect (G_OBJECT (w), "toggled",
				  G_CALLBACK (invert_toggle), gc);
	} else {
		w = gtk_button_new ();
		gtk_container_add (GTK_CONTAINER (w), l);
		g_signal_connect (G_OBJECT(w), "clicked",
				  but->signal_func,
				  (gpointer) but);
	}

	if (strcmp (but->name, "e") == 0)
		g_signal_connect (G_OBJECT (w), "button_press_event",
				  G_CALLBACK (maybe_run_slide_rule),
				  (gpointer) but);
	
	for(i=0;but->keys[i]!=0;i++) {
		gtk_widget_add_accelerator(w, "clicked",
					   gc->_priv->accel,
					   but->keys[i], 0,
					   GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(w, "clicked",
					   gc->_priv->accel,
					   but->keys[i],
					   GDK_SHIFT_MASK,
					   GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(w, "clicked",
					   gc->_priv->accel,
					   but->keys[i],
					   GDK_LOCK_MASK,
					   GTK_ACCEL_VISIBLE);
	}

	if (but->tooltip != NULL) {
		gtk_tooltips_set_tip(GTK_TOOLTIPS(gc->_priv->tooltips), w, _(but->tooltip), but->tooltip_private);
	}

	g_object_set_data(G_OBJECT(w), "set_data", gc);
	gtk_widget_show(w);
	gtk_table_attach(GTK_TABLE(table), w,
			 x, x+1, y, y+1,
			 GTK_FILL | GTK_EXPAND |
			 GTK_SHRINK,
			 GTK_FILL | GTK_EXPAND | GTK_SHRINK, 2, 2);

	/* if this is the DRG button, remember it's pointer */
	if(but->signal_func == G_CALLBACK (drg_toggle))
		gc->_priv->drg_button = w;
}

static void
gnome_calc_instance_init (GnomeCalc *gc)
{
	GtkWidget *hbox;
	GtkWidget *table;
	GtkTextBuffer *buffer;
	gint x,y;
	
	gc->_priv = g_new0(GnomeCalcPrivate, 1);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (gc), hbox, FALSE, FALSE, 0);
	
	gc->_priv->text_display = gtk_text_view_new ();
	gc->_priv->store_display = gtk_text_view_new ();

	g_object_set (G_OBJECT (gc->_priv->store_display), 
		      "cursor-visible", FALSE, NULL);
	
        gtk_text_view_set_editable (GTK_TEXT_VIEW (gc->_priv->store_display), FALSE);
        gtk_text_view_set_justification (GTK_TEXT_VIEW (gc->_priv->store_display),
                                         GTK_JUSTIFY_LEFT);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (gc->_priv->store_display),
                                     GTK_WRAP_WORD);
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (gc->_priv->store_display));
        gtk_text_buffer_create_tag (buffer, "x-large",
                                    "scale", PANGO_SCALE_XX_LARGE, NULL);
        gtk_box_pack_start (GTK_BOX (hbox), gc->_priv->store_display, TRUE, TRUE, 0);

        gtk_text_view_set_editable (GTK_TEXT_VIEW (gc->_priv->text_display), FALSE);
        gtk_text_view_set_justification (GTK_TEXT_VIEW (gc->_priv->text_display),
                                         GTK_JUSTIFY_RIGHT);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (gc->_priv->text_display),
                                     GTK_WRAP_WORD);
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (gc->_priv->text_display));
        gtk_text_buffer_create_tag (buffer, "x-large",
                                    "scale", PANGO_SCALE_XX_LARGE, NULL);
        gtk_box_pack_end (GTK_BOX (hbox), gc->_priv->text_display, TRUE, TRUE, 0);

	gtk_widget_show (gc->_priv->text_display);
	gtk_widget_show (gc->_priv->store_display);
	
	gc->_priv->stack = NULL;
	gc->_priv->result = 0;
	strcpy(gc->_priv->result_string," 0");
	gc->_priv->memory = 0;
	gc->_priv->mode = GNOME_CALC_DEG;
	gc->_priv->invert = FALSE;
	gc->_priv->add_digit = TRUE;
	gc->_priv->accel = gtk_accel_group_new();
	gc->_priv->tooltips = gtk_tooltips_new();

	table = gtk_table_new(8,5,TRUE);
	gtk_widget_show(table);

	gtk_box_pack_start(GTK_BOX(gc),table,TRUE,TRUE,0);

	for(x=0;x<5;x++) {
		for(y=0;y<8;y++) {
			create_button(gc, table, x, y);
		}
	}	
	
	gtk_tooltips_enable (gc->_priv->tooltips);

        set_display (gc);
	
}


/**
 * gnome_calc_new:
 *
 * Description: Creates a calculator widget, a window with all the common
 * buttons and functions found on a standard pocket calculator.
 *
 * Returns: Pointer to newly-created calculator widget.
 **/

GtkWidget *
gnome_calc_new (void)
{
	GnomeCalc *gcalculator;

	gcalculator = g_object_new (gnome_calc_get_type (), NULL);

	return GTK_WIDGET (gcalculator);
}

static void
gnome_calc_destroy (GtkObject *object)
{
	GnomeCalc *gc;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CALC (object));

	gc = GNOME_CALC (object);

	while(gc->_priv->stack)
		stack_pop(&gc->_priv->stack);

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_calc_finalize (GObject *object)
{
	GnomeCalc *gc;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CALC (object));

	gc = GNOME_CALC (object);

	if(gc->_priv) {
		g_free(gc->_priv);
		gc->_priv = NULL;
	}

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_calc_get_result
 * @gc: Pointer to GNOME calculator widget
 *
 * Description:  Gets the value of the result buffer as a double.
 * This should read off whatever is currently on the display of the
 * calculator.
 *
 * Returns:  A double precision result.
 **/

gdouble
gnome_calc_get_result (GnomeCalc *gc)
{
	g_return_val_if_fail (gc, 0.0);
	g_return_val_if_fail (GNOME_IS_CALC (gc), 0.0);

	return gc->_priv->result;
}

/**
 * gnome_calc_get_accel_group
 * @gc: Pointer to GNOME calculator widget
 *
 * Description:  Gets the accelerator group which you can add to 
 * the toplevel window or wherever you want to catch the keys for this
 * widget.
 *
 * Returns:  The accelerator group
 **/

GtkAccelGroup   *
gnome_calc_get_accel_group(GnomeCalc *gc)
{
	g_return_val_if_fail (gc, NULL);
	g_return_val_if_fail (GNOME_IS_CALC (gc), NULL);

	return gc->_priv->accel;
}
/**
 * gnome_calc_get_result_string:
 * @gc: Pointer to GNOME calculator widget
 *
 * Description:  Gets the internal string representation of the result.
 *
 * Returns:  Internal string pointer, do not free
 **/
const char *
gnome_calc_get_result_string(GnomeCalc *gc)
{
	g_return_val_if_fail (gc, NULL);
	g_return_val_if_fail (GNOME_IS_CALC (gc), NULL);

	return gc->_priv->result_string;
}




/**
 * delete_digit:
 * @w: 
 * @data: 
 * 
 * Handle the backspace event
 **/
static void
backspace_cb (GtkWidget *w, gpointer data)
{
	GnomeCalc *gc = g_object_get_data(G_OBJECT(w), "set_data");
	char *old_locale;
	gint length;

	g_return_if_fail (GNOME_IS_CALC (gc));

	if(gc->_priv->error)
		return;

	unselect_invert(gc);

	length = strlen (gc->_priv->result_string);

	/* The behaivor that I expect from the calculator when the backspace key
	 * is pressed is :
	 * - If i was typing a number, i expect to delete the last char
	 * - If i have a result already, i expect to clear the calculator
	 */
	if (!gc->_priv->add_digit) {
		clear_calc (w, data);
		return;
	}

	if (atof (gc->_priv->result_string) == 0.0)
		return;
	
	gc->_priv->result_string [length - 1] = 0;
	if (atof (gc->_priv->result_string) == 0.0) {
		gc->_priv->result_string [0] = ' ';
		gc->_priv->result_string [1] = '0';
		gc->_priv->result_string [2] = 0;
	}

	gc->_priv->result = atof (gc->_priv->result_string);
	if (gc->_priv->stack) {
		CalculatorStack *stack = gc->_priv->stack->data;
		if (stack->type == CALCULATOR_NUMBER)
			stack->d.number = gc->_priv->result;
	}

	set_display (gc);
 	
	/* make sure get values in a consistent manner */
	old_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
	setlocale (LC_NUMERIC, "C");
	sscanf(gc->_priv->result_string, "%lf", &gc->_priv->result);
	setlocale (LC_NUMERIC, old_locale);
	g_free (old_locale);
}

/*
 * This code adds the ability to bound other keys to certain functions
 * usefull for keys that can't be bound as an accelerator or functions
 * that do not have a button in the calculator. Chema
 */
typedef struct _GnomeCalcExtraKeys GnomeCalcExtraKeys;
typedef void (*CalcKeypressFunc) (GtkWidget *w, gpointer data);

struct _GnomeCalcExtraKeys {
	gint key;
	CalcKeypressFunc signal_func;
	gpointer data;
};

static const GnomeCalcExtraKeys extra_keys [] = {
	{GDK_BackSpace, backspace_cb, NULL},
	{GDK_Return,    no_func,      NULL},
	{GDK_KP_Enter,  no_func,      NULL},
	{GDK_Delete,    clear_calc,   NULL},
	{GDK_KP_Delete, add_digit,    "."},
	{GDK_KP_Left,   add_digit,    "4"},
	{GDK_KP_4,      add_digit,    "4"},
	{GDK_KP_Right,  add_digit,    "6"},
	{GDK_KP_6,      add_digit,    "6"},
	{GDK_KP_Up,     add_digit,    "8"},
	{GDK_KP_8,      add_digit,    "8"},
	{GDK_KP_Down,   add_digit,    "2"},
	{GDK_KP_2,      add_digit,    "2"}	
};
	
static gboolean
event_cb (GtkWidget *widget, GdkEvent *event, gpointer calc)
{
	GnomeCalc *gc;
	gint i, num;
	GdkEventKey *kevent;

	if (event->type != GDK_KEY_PRESS)
		return FALSE;

	kevent = (GdkEventKey *)event;

	gc = (GnomeCalc *) calc;
	g_return_val_if_fail (GNOME_IS_CALC (gc), FALSE);

	num = sizeof (extra_keys) / sizeof (GnomeCalcExtraKeys);

	for (i = 0; i < num; i++) {
		if (kevent->keyval == extra_keys[i].key) {
			CalculatorButton but = {NULL};
			CalcKeypressFunc func = extra_keys [i].signal_func;
			but.data = extra_keys[i].data;
			(* func) (widget, &but);
			return TRUE;
		}
	}

	return FALSE;
}


void
gnome_calc_bind_extra_keys (GnomeCalc *gc,
			    GtkWidget *widget)
{
	g_return_if_fail (GNOME_IS_CALC (gc));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	g_object_set_data (G_OBJECT (widget), "set_data", gc);
	
	g_signal_connect (GTK_OBJECT (widget), "event",
			  G_CALLBACK (event_cb), gc);

}
