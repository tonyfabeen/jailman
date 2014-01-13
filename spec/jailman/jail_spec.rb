require 'spec_helper'
require 'jailman/jail'

describe Jailman::Jail do

  context 'when initialized' do
    let(:jail) { described_class.new }

    it 'populates directory' do
      expect(jail.directory).to_not be_nil
      expect(jail.directory).to match(Dir.pwd)
    end

  end
end
