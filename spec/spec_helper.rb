require 'bundler'
Bundler.setup(:default, :development)

$: << File.dirname(__FILE__) + '/../ext/jailman_c'

RSpec.configure do |config|
  config.order = 'random'

  config.expect_with :rspec do |c|
    c.syntax = [:expect]
  end

end
