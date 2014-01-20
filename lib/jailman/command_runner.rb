#encoding: utf-8
module Jailman
  class CommandRunner

    def self.run(command)
      output = `#{command}`
      raise output unless $? == 0
      output
    end

  end
end
