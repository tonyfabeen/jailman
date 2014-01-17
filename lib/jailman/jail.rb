#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name
    attr_reader   :configuration

    def initialize(name=nil, directory=nil)
      @directory = directory || Dir.pwd
      @name      = name || @directory.split('/').last
    end

    def create
      provisioner = Jailman::Provisioner.new(self)
      provisioner.run!

      configuration = Jailman::Configuration.new(self)
      configuration.create_yaml
    end

  end

end
