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
package com.db4o.replication;

import com.db4o.*;
import com.db4o.query.*;


/**
 * db4o replication interface.
 * @deprecated Since db4o-5.2. Use db4o Replication System (dRS)
 * instead.<br><br>
 * @see com.db4o.ext.ExtObjectContainer#replicationBegin(ObjectContainer, ReplicationConflictHandler)
 */
public interface ReplicationProcess {
	
	/**
	 * checks if an object has been modified in both ObjectContainers involved
	 * in the replication process since the last time the two ObjectContainers
	 * were replicated.
	 * @param obj - the object to check for a conflict.
	 */
	public void checkConflict(Object obj);
    
    /**
     * commits the replication task to both involved ObjectContainers.
     * <br><br>Call this method after replication is completed to
     * write all changes back to the database files. This method
     * synchronizes both ObjectContainers by setting the transaction
     * serial number {@link com.db4o.ext.ExtObjectContainer#version()} on both 
     * ObjectContainers to be equal 
     * to the higher version number among the two. A record with 
     * information about this replication task, including the 
     * synchronized version number is stored to both ObjectContainers
     * to allow future incremental replication.
     */
    public void commit();
	
	/**
	 * returns the "peerA" ObjectContainer involved in this ReplicationProcess. 
	 */
	public ObjectContainer peerA();
	
	
	/**
	 * returns the "peerB" ObjectContainer involved in this ReplicationProcess. 
	 */
	public ObjectContainer peerB();
	
    
	/**
     * replicates an object.
     * <br><br>By default the version number of the object is checked in
     * both ObjectContainers involved in the replication process. If the
     * version number has not changed since the last time the two
     * ObjectContainers were replicated 
     * @param obj
     */
    public void replicate(Object obj);
    
    /**
     * ends a replication task without committing any changes.
     */
    public void rollback();
    
    /**
     * modifies the replication policy, what to do on a call to {@link #replicate(Object)}. 
     * <br><br>If no direction is set, the replication process will be bidirectional by
     * default.
     * @param relicateFrom the ObjectContainer to replicate from 
     * @param replicateTo the ObjectContainer to replicate to 
     */
    public void setDirection(ObjectContainer relicateFrom, ObjectContainer replicateTo);

    /**
	 * adds a constraint to the passed Query to query only for objects that
	 * were modified since the last replication process between the two
	 * ObjectContainers involved in this replication process.
	 * @param query the Query to be constrained 
	 */
	public void whereModified(Query query);
	

}
