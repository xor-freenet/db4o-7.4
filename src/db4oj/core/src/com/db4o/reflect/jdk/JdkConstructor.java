/* Copyright (C) 2004 - 2008  db4objects Inc.  http://www.db4o.com

This file is part of the db4o open source object database.

db4o is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published
by the Free Software Foundation and as clarified by db4objects' GPL 
interpretation policy, available at
http://www.db4o.com/about/company/legalpolicies/gplinterpretation/
Alternatively you can write to db4objects, Inc., 1900 S Norfolk Street,
Suite 350, San Mateo, CA 94403, USA.

db4o is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */
package com.db4o.reflect.jdk;

import java.lang.reflect.*;

import com.db4o.*;
import com.db4o.internal.*;
import com.db4o.reflect.*;
import com.db4o.reflect.core.*;

/**
 * Reflection implementation for Constructor to map to JDK reflection.
 * 
 * @sharpen.ignore
 */
public class JdkConstructor implements ReflectConstructor{
	
	private final Reflector reflector;
	private final Constructor constructor;
	
	public JdkConstructor(Reflector reflector_, Constructor constructor_){
		reflector = reflector_;
		constructor = constructor_;
		Platform4.setAccessible(constructor);
	}
	
	public ReflectClass[] getParameterTypes(){
		return JdkReflector.toMeta(reflector, constructor.getParameterTypes());
	}
	
	public Object newInstance(Object[] parameters) {
		Object obj = null;
		try {
			obj = constructor.newInstance(parameters);
			if (DTrace.enabled) {
				DTrace.NEW_INSTANCE.log(System.identityHashCode(obj));
			}
		} catch (LinkageError e) {
			// e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// e.printStackTrace();
		} catch (InstantiationException e) {
			// e.printStackTrace();
		} catch (IllegalAccessException e) {
			// e.printStackTrace();
		} catch (InvocationTargetException e) {
			// e.printStackTrace();
		}
		return obj;
	}
}
