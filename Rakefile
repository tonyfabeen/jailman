require "bundler/gem_tasks"
require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new('spec')

task :build do
  Dir.chdir('deps/program_space') do
    output = `mkdir -p /var/local/jailman/jails`
    raise output unless $? == 0
    output = `mkdir -p /var/run/jailman/jails`
    raise output unless $? == 0
    output = `make`
    raise output unless $? == 0
    output = `cp psc /usr/local/bin/psc`
    raise output unless $? == 0
    output = `cp ps_rootfs /usr/local/bin/ps_rootfs`
    raise output unless $? == 0
    end
end

task :spec => :build

task :default => :spec

