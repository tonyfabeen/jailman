#encoding: utf-8
require 'erb'
require 'yaml'

module Jailman
  class Configuration

    TEMPLATE = <<-YAML
      application:
        name : <%= @jail.name %>
        repository: # git@github.com:tonyfabeen/jailman.git
      run:
        commands:
          # Here you put your list of commands
          #- bundle install
          #- bundle exec rails s -p 8888
    YAML

    attr_accessor :jail, :yaml

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

    def create_yaml
      build
      File.open("#{jail.directory}/jail.yml", 'w') do |f|
        f.puts @yaml_file
      end
    end

    def build
      template = ERB.new(TEMPLATE)
      @yaml_file = template.result(binding)
    end

  end
end
