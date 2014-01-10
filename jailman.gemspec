# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'jailman/version'

Gem::Specification.new do |spec|
  spec.name          = "jailman"
  spec.version       = Jailman::VERSION
  spec.authors       = ["Tony Fabeen"]
  spec.email         = ["tony.fabeen@gmail.com"]
  spec.description   = "Jailman jail your app for you"
  spec.summary       = "Jailman jail your app for you"
  spec.homepage      = ""
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib", "ext/jailman_c"]
  spec.extensions    = ["ext/jailman_c/extconf.rb"]

  spec.add_development_dependency "bundler", "~> 1.3"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rspec"
  spec.add_development_dependency "rspec-expectations"

  spec.version = Jailman::VERSION
end
