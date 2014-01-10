require 'bundler'
Bundler.setup(:default, :development)

RSpec.configure do |config|
  config.order = 'random'

  config.expect_with :rspec do |c|
    c.syntax = [:expect]
  end

end
