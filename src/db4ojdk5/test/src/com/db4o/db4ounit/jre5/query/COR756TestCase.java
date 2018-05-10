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
package com.db4o.db4ounit.jre5.query;

import java.io.*;

import com.db4o.*;
import com.db4o.query.*;

import db4ounit.*;
import db4ounit.extensions.*;

public class COR756TestCase extends AbstractDb4oTestCase {
	
	public static class A implements Serializable {
		
		private static final long serialVersionUID = -3347486295375853187L;
		public boolean GetAllB(B b){
			return b.a == this;
		}
		public A() {
			
		}
	}
	
	public static class B implements Serializable {
		private static final long serialVersionUID = 1778716574622308900L;
		
		public A a;
		public B() {
		}
	}
	
	public static class Evaluator implements Evaluation{
		public A _a;
		public Evaluator(A a) {
			_a = a;
		}
	   public void evaluate(Candidate candidate){
		    candidate.include(((B)candidate.getObject()).a == _a);
	  }
				
	}
	
	public static class DeepTwoEvaluator implements Evaluation{
		private static final long serialVersionUID = 1L;
		
		public static class C {
			A _a;
			public C(A a) {
				_a = a;
			}
		}

		public C _c;
		
		public DeepTwoEvaluator(C c) {
			_c = c;
		}
		
	   public void evaluate(Candidate candidate){
		    candidate.include(((B)candidate.getObject()).a == _c._a);
	   }
		
	}
	
	public static class CircularEvaluator implements Evaluation	{
		private static final long serialVersionUID = 1L;

		public static class D {
			D _d;
			A _a;
			public D(D d, A a) {
				_d = d;
				_a = a;
			}
		}
		
		D _d;
		
		public CircularEvaluator(D d) {
			this._d = d;
		}
		
		public void evaluate(Candidate candidate) {
			candidate.include(((B)candidate.getObject()).a == _d._a);
		}
	}
	
	
	public static void main(String[] args) {
		new COR756TestCase().runAll();
	}
	
	private A _a;
	private B _b;
	

	@SuppressWarnings("unchecked")
	public void testSimpleSODA() throws Exception {
		populate();
		ObjectSet set = executeSODAQuery(_a, new Evaluator(_a));		
		Assert.areEqual(1, set.size());		
	}

	// It will fail for C/S because the reference system in the
	// server is not in sync.
	@SuppressWarnings("unchecked")
	public void _testChangingSODA() throws Exception {
		populate();
		_b.a = new A();
		
		ObjectSet set = executeSODAQuery(_a, new Evaluator(_a));
		Assert.areEqual(0, set.size());

		set = executeSODAQuery(_b.a, new Evaluator(_b.a));
		Assert.areEqual(1, set.size());

	}
	
	@SuppressWarnings("unchecked")
	public void testDeepTwoEvaluation() throws Exception {
		populate();
		ObjectSet set = executeSODAQuery(_a, 
				new DeepTwoEvaluator( 
						new DeepTwoEvaluator.C(_a)));		
		Assert.areEqual(1, set.size());		
	}
	
	@SuppressWarnings("unchecked")
	public void testCircularEvaluation() throws Exception {
		populate();
		CircularEvaluator.D one = new CircularEvaluator.D(null, _a);
		CircularEvaluator.D two = new CircularEvaluator.D(one, _a);
		one._d = two;
		ObjectSet set = executeSODAQuery(_a, new CircularEvaluator(one));		
		Assert.areEqual(1, set.size());		
	}	

	@SuppressWarnings("unchecked")
	private ObjectSet executeSODAQuery(final A a, Evaluation e) {
		Query q = db().query();
        q.constrain(e);
		ObjectSet set = q.execute();
		return set;
	}
	
	private void populate() {
		_a = new A();
		_b = new B();
		_b.a = _a;
		store(_b);
	}
	
	
}
