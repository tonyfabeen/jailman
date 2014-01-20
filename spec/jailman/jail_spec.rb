#encoding: utf-8
require 'spec_helper'
require 'jailman/jail'

describe Jailman::Jail do

  context 'when initialized' do

    context 'without a name' do
      let(:jail) { described_class.new }

      it 'populates directory' do
        expect(jail.directory).to_not be_nil
        expect(jail.directory).to match(Dir.pwd)
      end

      it 'and populates name with the last element of directory' do
        expect(jail.name).to_not be_nil
        expect(jail.name).to match (jail.directory.split('/').last)
      end

    end

    context 'with a name' do
      let(:jail) { described_class.new('my_jail') }

      it 'populates directory' do
        expect(jail.directory).to_not be_nil
        expect(jail.directory).to match(Dir.pwd)
      end

      it 'and populates name with the constructor argument' do
        expect(jail.name).to match('my_jail')
      end

    end

  end

  describe '#create' do

    let(:jail) do
      jail = jail_factory
      jail.create
      jail
    end

    it 'runs a jail and be able to receive commands' do
      script = "psc #{jail.name} --run free -m"
      output = `#{script}`
      expect(output).to match("Mem:")
    end

   after do
     script = "psc #{jail.name} --kill"
     output = `#{script}`
   end


  end
end
