#encoding: utf-8
require 'spec_helper'
require 'jailman/state'

describe Jailman::State do
  let(:jail) {jail_factory}

  context 'when initialized' do

    context 'whithout a jail' do

      it 'raises an exception' do
        expect { described_class.new }.to raise_error("jail must be present")
      end

    end

    context 'with a jail' do

      it 'initializes correctly' do
        state = described_class.new(jail)
        expect(state.jail).to_not be_nil
      end

    end

  end

  context '#save' do

    context 'when a jail not present' do
      it 'raises an exception' do
        expect { described_class.new.save }.to raise_error("jail must be present")
      end
    end

    context 'when a jail present' do
      let(:state) do
        state = described_class.new(jail)
        state.save
        state
      end

      it 'creates a json representing state' do
        expect(File.exists?(state.file_path)).to be_true
      end

      it 'json contains jail data' do
        json = JSON.parse(File.read(state.file_path))

        expect(json["name"]).to match(jail.name)
        expect(json["directory"]).to match(jail.directory)
      end

      after { FileUtils.rm_rf(state.file_path) }

    end

  end

  describe '#load' do
    context 'when a jail not present' do
      it 'raises an exception' do
        expect { described_class.new.load }.to raise_error("jail must be present")
      end
    end

    context 'when a jail is present' do

      let(:state) { described_class.new(jail) }

      context 'and file not saved' do

        it 'returns nil' do
          expect(state.load).to be_nil
        end

      end

      context 'and a file is saved' do
        it 'returns a jail instance' do
          state.save
          loaded_jail = state.load

          expect(loaded_jail.name).to match(jail.name)
          expect(loaded_jail.directory).to match(jail.directory)
        end
      end

    end

  end



end
