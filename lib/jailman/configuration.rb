#encoding: utf-8
require 'erb'

module Jailman
  class Configuration

    TEMPLATE = <<-YAML
      application_name : <%= @jail.name %>
      run:
        commands:
          # Here you put your list of commands
          #- bundle install
          #- bundle exec rails s -p 8888
    YAML


    attr_accessor :jail

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

    def create_file

    end

    def build_file
      template = ERB.new(TEMPLATE)
      template.result(binding)
    end

  end
end
