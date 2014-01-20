#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name
    attr_reader   :configuration
    attr_reader   :status

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

    def run(command)
      command = "#{Jailman::Constants::JAIL_SCRIPT} #{name} --run #{command}"
      Jailman::CommandRunner.run(command)
    end

    def status
      #TODO: NOw
    end


    def stop
      provisioner.stop!
      @status = "STOPPED"
    end



  end

end
