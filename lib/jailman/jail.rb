#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name
    attr_reader   :configuration

    def initialize(name=nil)
      @directory = Dir.pwd
      @name      = name || @directory.split('/').last
    end

    def create
      configuration = Jailman::Configuration.new(self)
      configuration.create_yaml

      provisioner = Jailman::Provisioner.new(self)
      provisioner.run!
   end

  end

end
