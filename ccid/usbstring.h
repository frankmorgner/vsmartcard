/*
 * (c) Copyright 2003 by David Brownell
 * All Rights Reserved.
 *
 * This software is licensed under the GNU LGPL version 2.
 */

/* utility to simplify dealing with string descriptors */

/**
 * struct usb_string - wraps a C string and its USB id
 * @id: the (nonzero) ID for this string
 * @s: the string, in UTF-8 encoding
 *
 * If you're using usb_gadget_get_string(), use this to wrap a string
 * together with its ID.
 */
struct usb_string {
	__u8			id;
	const char		*s;
};

/**
 * struct usb_gadget_strings - a set of USB strings in a given language
 * @language: identifies the strings' language (0x0409 for en-us)
 * @strings: array of strings with their ids
 *
 * If you're using usb_gadget_get_string(), use this to wrap all the
 * strings for a given language.
 */
struct usb_gadget_strings {
	__u16			language;	/* 0x0409 for en-us */
	struct usb_string	*strings;
};

/* put descriptor for string with that id into buf (buflen >= 256) */
int usb_gadget_get_string (struct usb_gadget_strings *table, int id, __u8 *buf);

