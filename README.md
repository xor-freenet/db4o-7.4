### db4o-7.4 - as needed by Freenet's plugin-WebOfTrust and plugin-Freetalk

This repository contains version 7.4.63.11890 of db4o as it was shipped
by Freenet's contrib repository, aka "freenet-ext.jar", as of tag v29.

freenet-ext.jar used to be a monolithic JAR intended to ship all of
Freenet's dependencies. As Freenet is nowadays capable of shipping
individual JARs freenet-ext.jar is going away.  
As a consequence db4o is being moved into this repository so it can be
directly compiled into WebOfTrust and Freetalk.

This is NOT the latest version of db4o!  
Instead it is what has been deployed at Freenet from at least 2011 to
2018 and what has proven to work well with WebOfTrust and Freetalk.
Later versions of db4o have been tried but caused severe bugs which
must first be fixed before we upgrade.

NOTICE: This doesn't include the commit history of db4o as it is not
based on an official db4o repository and thus should not be propagated
on the Internet beyond Freenet usage!  
This is because to ensure the code is the very same version as what has
proven to work well with Freenet it is a fork of Freenet's contrib
repository with anything besides db4o removed. This may or may not be
equal to what db4o has once shipped officially.

### Building

In order to ease validating that this produces a build which is
identical to what was shipped with freenet-ext this does not use Gradle
yet but ships the same Ant builder as used by freenet-ext.  
Thus you can build using:

```shell
ant clean
ant jar
```

### Build output

- ```db4o.jar```: The main output.
- ```build/```: Classes of ```db4o.jar```
- ```build-db4oj/```: Db4o main classes without optimizations.
  These are copied into the ```build/``` directory by the build script.
- ```build-db4ojdk*/```: Optimizations. The JDK version in their name is
  the minimum Java version required for each.
  They are copied into the ```build/``` directory by the build script.

### TODOs

- Check whether the class files which this produces are binary equal to
  what freenet-ext.jar of contrib tag v29 contains.  
  To validate that do a fresh compile of freenet-ext.jar on the same
  machine.

- While the unit test files are present they are currently ignored by the
  Ant builder.  
  This is because whoever committed db4o to the repository did not include
  db4o's "db4ounit" library which is needed for the tests.  
  The code of it should be added to this repository and the Ant builder
  should be amended to run the unit tests.  
  However this should IMHO be postponed until after the initial release
  of Freenet without contrib / of WoT with bundled db4o:  
  There likely is a reason that whoever added db4o to contrib left out
  the tests, and that reason may be that they fail frequently. The
  complex procedure of getting rid of contrib at Freenet should not
  unnecessarily be complicated by introducing fragile tests here.
