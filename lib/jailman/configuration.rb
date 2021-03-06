#encoding: utf-8
require 'erb'
require 'yaml'

module Jailman
  class Configuration

    attr_accessor :jail, :file_path
    attr_reader :application_name
    attr_reader :application_repo
    attr_reader :commands

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

    def create_yaml
      build

      @file_path = "#{jail.directory}/jail.yml"

      f = File.open(@file_path, 'w+')
      f.puts @yaml_file
      f.close
    end

    def read
      yaml_data         = YAML.load_file(file_path)
      @application_name = yaml_data["application"]["name"]
      @application_repo = yaml_data["application"]["repository"]
      @commands         = yaml_data["run"]["commands"]
    end

    private

    def build
      template = ERB.new(config_template)
      @yaml_file = template.result(binding)
    end

    def config_template
      template = <<-YAML
        application:
          name : <%= @jail.name %>
          repository: # git@github.com:tonyfabeen/jailman.git
        run:
          commands:
            # Here you put your list of commands
            #- bundle install
            #- bundle exec rails s -p 8888
      YAML

      template
    end

  end
end
