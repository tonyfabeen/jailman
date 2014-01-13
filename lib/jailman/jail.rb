#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name
    attr_reader   :configuration

    def initialize(name=nil)
      @directory = Dir.pwd
      @name      = name || @directory.split('/').last
      @configuration = Jailman::Configuration.new(self)
    end

    def create
      configuration.create_yaml
    end

  end

end
