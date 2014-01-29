# Jailman

It's a Linux Container Runtime, written with a bit of C and a lot of Ruby.

It aims to do the same of LXC, but with less resources, to be pluggable and
more devops friendly.

Notes : It's currently in alpha version not yet read for production.
        All tests are running on Ubuntu > 12.04 versions.

Enjoy!

## Pre-Requisites

On Ubuntu you must first prepare your network to support bridges:

First install the `brigde-utils` package:

```
  $ sudo apt-get install -y bridge-utils
```

Then edit the file `/etc/network/interfaces`:

```
#auto lo
#iface lo inet loopback

auto br0
iface br0 inet dhcp
    bridge_ports eth0
    bridge_fd 0
    bridge_maxwait 0
```

And finally restart the network:

```
$ sudo /etc/init.d/networking restart
```

## Installation


Jailman requires sudo privileges. To install type :

    $ sudo gem install jailman

## Usage

###Starting a new Jail :

```
$ sudo jail new first ~/.jails/first

[JAILMAN] Creating Jail first
[JAILMAN] first created

```

* It creates a new Jail called 'first'.
* It creates a new Root Filesystem at '~/.jails/first'.
* It generates a Config File at '~/.jails/first'

``` yaml

application:
  name : [Application Name]
  repository: [Git repository Url]
run:
  commands:
    # Here you put your list of commands
    #- bundle install
    #- bundle exec rails s -p 8888

```

### Running a command inside a Jail

```
$ sudo jail runner first 'free -m'

total       used       free     shared    buffers     cached
Mem:      16319304    4571076   11748228          0     293064    1617360
-/+ buffers/cache:    2660652   13658652
Swap:     33318908          0   33318908

```

###List Jails

```
$ sudo jail list

+-------+-------+-------------------------------+---------+
| PID   | NAME  | DIRECTORY                     | STATUS  |
| 18600 | first | /home/user/.jails/first       | STARTED |
+-------+-------+-------------------------------+---------+

```

###Destroy a Jail

```
$ sudo jail destroy first

[JAILMAN] Destroy Container : first
[JAILMAN] first destroyed

```



## Running Tests

It requires sudo privileges.

```
$ sudo rake spec
```

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

