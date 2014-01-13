# Jailman

Jails your app for you

## Installation

Add this line to your application's Gemfile:

    gem 'jailman'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install jailman

## Usage

###Creating a new Jail :

Into your application directory :

```
    jail new [app-name]
```

It creates a jail.yml where you can edit your jail configuration.

``` yaml
application_name : [app-name]
run:
  commands:
    - bundle install
    - bundle exec rails s -p 8888
```

###Starting a jail:

Into your application directory :

```
    jail start
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

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

