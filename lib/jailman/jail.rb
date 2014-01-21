#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name
    attr_reader   :configuration, :provisioner

    def initialize(name=nil, directory=nil)
      @directory = directory || Jailman::Constants::ROOTFS_DIR
      @name      = name || @directory.split('/').last
      @provisioner = Jailman::Provisioner.new(self)
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
     running? ? "STARTED" : "STOPPED"
    end

    def running?
      File.exists?("#{Jailman::Constants::PID_DIR}/#{name}.pid")
    end

    def stop
      @provisioner.stop!
    end

    def self.find(name)
    end



  end

end
