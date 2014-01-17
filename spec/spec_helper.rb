require 'bundler'
Bundler.setup(:default, :development)

RSpec.configure do |config|
  config.order = 'random'

  config.expect_with :rspec do |c|
    c.syntax = [:expect]
  end

end

def jail_factory
  jail = Jailman::Jail.new("Jail#{rand(1000)}")
  jail.directory = Dir.pwd + "/spec/fixtures/jails/#{jail.name}"
  jail
end

