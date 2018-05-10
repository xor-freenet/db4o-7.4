### db4o-7.4 - as needed by Freenet's plugin-WebOfTrust and plugin-Freetalk.

This repository contains version 7.4.63.11890 of db4o as it was shipped
by Freenet's contrib repository, aka "freenet-ext.jar", as of tag v29.

That's what has been deployed on the Freenet network from at least 2011
to 2018 and what has proven to work well with WebOfTrust and Freetalk.

freenet-ext.jar used to be a monolithic JAR intended to ship all of
Freenet's dependencies. As Freenet is nowadays capable of shipping
individual JARs freenet-ext.jar is going away.

As a consequence db4o is being moved into this repository so it can be
compiled into WebOfTrust and Freetalk directly as they rely heavily on
db4o and removing its usage would be at least a year of work for each.

NOTICE: This doesn't include the commit history of db4o. Instead to
ensure the code is the very same version as what has proven to work well
it is a fork of Freenet's contrib repository with anything besides db4o
removed.

### Building
```shell
ant clean
ant jar
```

### Build output
- ```db4o.jar```: The main output.
- ```build/``` directory: Classes of ```db4o.jar```
- ```build-*/``` directories: Optimizations. They are copied into the
  ```build/``` directory by the build script. The JDK version in their
  name is the minimum Java version required for each.
