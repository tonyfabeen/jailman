require "bundler/gem_tasks"
require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new('spec')

task :build do
  Dir.chdir('deps/program_space') do
    output = `make`
    raise output unless $? == 0
    output = `cp psc ../bin`
    raise output unless $? == 0
  end
end

task :spec => :build

task :default => :spec

