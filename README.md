# Jailman

It's a Linux Container Runtime, written with a bit of C and a lot of Ruby.

It aims to do the same of LXC, but with less resources, to be pluggable and
more devops friendly.

Enjoy!

## Installation

Jailman requires sudo privileges. To install type :

    $ sudo gem install jailman

## Usage

###Creating a new Jail :


```
  sudo jail new beer /path/to/beer
```

* It creates a new Jail called 'beer'.
* It creates a new Root Filesystem at '/path/to/beer'.
* It generates a Config File at '/path/to/beer'

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

###Starting a jail:

Into your application directory :

```
  jail start [jail_name]
```

###Killing a Jail

```
  jail kill [app-name]
```

It will kill your jail process.

###List existing Jails

```
  jail list
```

## Running Tests

It requires sudo privileges.

```
  sudo rake spec
```

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

